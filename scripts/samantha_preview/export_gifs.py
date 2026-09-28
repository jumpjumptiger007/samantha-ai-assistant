#!/usr/bin/env python3
"""Serve the fixed-step browser exporter and encode its RGBA frames as GIFs."""

from __future__ import annotations

import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[2]
PREVIEW = ROOT / "scripts" / "samantha_preview"
STATIC_FILES = {
    "/scripts/samantha_preview/export.html": PREVIEW / "export.html",
    "/scripts/samantha_preview/export.js": PREVIEW / "export.js",
    "/scripts/samantha_preview/preview.js": PREVIEW / "preview.js",
}
OUTPUTS = {
    "assets/samantha/idle.gif": ROOT / "assets" / "samantha" / "idle.gif",
    "assets/samantha/listening.gif": ROOT / "assets" / "samantha" / "listening.gif",
    "assets/samantha/thinking.gif": ROOT / "assets" / "samantha" / "thinking.gif",
    "assets/samantha/speaking.gif": ROOT / "assets" / "samantha" / "speaking.gif",
    "docs/samantha-ui-demo.gif": ROOT / "docs" / "samantha-ui-demo.gif",
}
for output_name, output_path in OUTPUTS.items():
    STATIC_FILES[f"/{output_name}"] = output_path
BACKGROUND = (0xD1, 0x68, 0x4E)
FOREGROUND = (0xFF, 0xFF, 0xFF)
FPS = 20


def _palette() -> bytes:
    colors = bytearray()
    for step in range(256):
        colors.extend(
            round(start + (end - start) * step / 255)
            for start, end in zip(BACKGROUND, FOREGROUND)
        )
    return bytes(colors)


def _quantize_frame(rgba: bytes | bytearray | memoryview, width: int, height: int) -> bytes:
    denominator = sum((end - start) ** 2 for start, end in zip(BACKGROUND, FOREGROUND))
    red = [(value - BACKGROUND[0]) * (FOREGROUND[0] - BACKGROUND[0]) for value in range(256)]
    green = [(value - BACKGROUND[1]) * (FOREGROUND[1] - BACKGROUND[1]) for value in range(256)]
    blue = [(value - BACKGROUND[2]) * (FOREGROUND[2] - BACKGROUND[2]) for value in range(256)]
    row_bytes = width * 4
    indexed = bytearray(width * height)
    destination = 0

    for output_y in range(height):
        source = (height - output_y - 1) * row_bytes
        for _ in range(width):
            projection = red[rgba[source]] + green[rgba[source + 1]] + blue[rgba[source + 2]]
            if projection <= 0:
                indexed[destination] = 0
            elif projection >= denominator:
                indexed[destination] = 255
            else:
                indexed[destination] = (projection * 255 + denominator // 2) // denominator
            source += 4
            destination += 1
    return bytes(indexed)


def _lzw_encode(indices: bytes) -> bytes:
    clear_code = 256
    end_code = 257
    dictionary: dict[int, int] = {}
    next_code = 258
    code_size = 9
    packed = bytearray()
    bit_buffer = 0
    bit_count = 0

    def write_code(code: int) -> None:
        nonlocal bit_buffer, bit_count
        bit_buffer |= code << bit_count
        bit_count += code_size
        while bit_count >= 8:
            packed.append(bit_buffer & 0xFF)
            bit_buffer >>= 8
            bit_count -= 8

    write_code(clear_code)
    if not indices:
        write_code(end_code)
    else:
        prefix = indices[0]
        for symbol in indices[1:]:
            key = (prefix << 8) | symbol
            found = dictionary.get(key)
            if found is not None:
                prefix = found
                continue

            write_code(prefix)
            if next_code < 4096:
                dictionary[key] = next_code
                next_code += 1
                if next_code > (1 << code_size) and code_size < 12:
                    code_size += 1
            else:
                write_code(clear_code)
                dictionary.clear()
                next_code = 258
                code_size = 9
            prefix = symbol
        write_code(prefix)
        write_code(end_code)

    if bit_count:
        packed.append(bit_buffer & 0xFF)
    return bytes(packed)


def _sub_blocks(data: bytes) -> bytes:
    blocks = bytearray()
    for start in range(0, len(data), 255):
        block = data[start : start + 255]
        blocks.append(len(block))
        blocks.extend(block)
    blocks.append(0)
    return bytes(blocks)


def encode_gif(
    raw_frames: bytes,
    width: int,
    height: int,
    frame_count: int,
    fps: int,
    looping: bool,
) -> bytes:
    if width < 1 or height < 1 or width > 240 or height > 320:
        raise ValueError("unsupported frame dimensions")
    if fps != FPS:
        raise ValueError(f"export rate must remain {FPS} FPS")
    if frame_count < 1 or frame_count > 1600:
        raise ValueError("unsupported frame count")
    expected_bytes = width * height * 4 * frame_count
    if len(raw_frames) != expected_bytes:
        raise ValueError(f"expected {expected_bytes} RGBA bytes, got {len(raw_frames)}")

    delay_cs = round(100 / fps)
    output = bytearray(b"GIF89a")
    output.extend(width.to_bytes(2, "little"))
    output.extend(height.to_bytes(2, "little"))
    output.extend((0xF7, 0, 0))  # 256-entry global color table, background index 0.
    output.extend(_palette())

    if looping:
        output.extend(b"\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00")

    frame_bytes = width * height * 4
    for frame_index in range(frame_count):
        start = frame_index * frame_bytes
        indexed = _quantize_frame(memoryview(raw_frames)[start : start + frame_bytes], width, height)
        compressed = _lzw_encode(indexed)
        output.extend(b"\x21\xf9\x04\x04")  # Keep the opaque full-frame canvas.
        output.extend(delay_cs.to_bytes(2, "little"))
        output.extend(b"\x00\x00")
        output.append(0x2C)
        output.extend(b"\x00\x00\x00\x00")
        output.extend(width.to_bytes(2, "little"))
        output.extend(height.to_bytes(2, "little"))
        output.append(0)
        output.append(8)  # LZW minimum code size for the 256-color table.
        output.extend(_sub_blocks(compressed))
    output.append(0x3B)
    return bytes(output)


class ExportHandler(BaseHTTPRequestHandler):
    server_version = "SamanthaPreviewExporter/1.0"

    def do_GET(self) -> None:  # noqa: N802 - stdlib handler API
        request_path = urlsplit(self.path).path
        if request_path == "/":
            self.send_response(302)
            self.send_header("Location", "/scripts/samantha_preview/export.html")
            self.end_headers()
            return
        if request_path == "/__health":
            self._send(200, b"ok", "text/plain; charset=utf-8")
            return
        source = STATIC_FILES.get(request_path)
        if source is None or not source.is_file():
            self._send(404, b"not found", "text/plain; charset=utf-8")
            return
        if source.suffix == ".html":
            content_type = "text/html; charset=utf-8"
        elif source.suffix == ".js":
            content_type = "text/javascript; charset=utf-8"
        else:
            content_type = "image/gif"
        self._send(200, source.read_bytes(), content_type)

    def do_POST(self) -> None:  # noqa: N802 - stdlib handler API
        if self.path != "/__export":
            self._send(404, b"not found", "text/plain; charset=utf-8")
            return
        try:
            name = self.headers.get("X-Asset-Name", "")
            if name not in OUTPUTS:
                raise ValueError("unknown output name")
            width = int(self.headers.get("X-Frame-Width", "0"))
            height = int(self.headers.get("X-Frame-Height", "0"))
            frame_count = int(self.headers.get("X-Frame-Count", "0"))
            fps = int(self.headers.get("X-Frame-Rate", "0"))
            looping = self.headers.get("X-Loop", "").lower() == "true"
            length = int(self.headers.get("Content-Length", "-1"))
            if length < 0 or length > 100_000_000:
                raise ValueError("invalid export payload size")
            raw_frames = self.rfile.read(length)
            encoded = encode_gif(raw_frames, width, height, frame_count, fps, looping)
            destination = OUTPUTS[name]
            destination.parent.mkdir(parents=True, exist_ok=True)
            temporary = destination.with_suffix(destination.suffix + ".tmp")
            temporary.write_bytes(encoded)
            temporary.replace(destination)
            print(f"Wrote {name}: {frame_count} frames, {len(encoded)} bytes", flush=True)
            response = json.dumps({"path": name, "frames": frame_count, "bytes": len(encoded)}).encode()
            self._send(200, response, "application/json")
        except (ValueError, OSError) as error:
            response = json.dumps({"error": str(error)}).encode()
            self._send(400, response, "application/json")

    def _send(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8767)
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), ExportHandler)
    print(f"Open http://127.0.0.1:{args.port}/ and choose Export all GIFs.", flush=True)
    print("The server writes only the five documented Samantha GIF outputs.", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("Exporter stopped.", flush=True)
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
