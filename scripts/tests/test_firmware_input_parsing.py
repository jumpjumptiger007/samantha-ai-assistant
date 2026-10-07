import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
CJSON_INCLUDE = ROOT / "managed_components/espressif__cjson/cJSON"


class FirmwareInputParsingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        c_compiler = shlex.split(os.environ.get("CC", "cc"))
        if not compiler or shutil.which(compiler[0]) is None:
            raise unittest.SkipTest("host C++ compiler is unavailable")
        if not c_compiler or shutil.which(c_compiler[0]) is None:
            raise unittest.SkipTest("host C compiler is unavailable")
        if not (CJSON_INCLUDE / "cJSON.h").is_file():
            raise unittest.SkipTest("ESP-IDF cJSON headers are unavailable")

        cls.temp_dir = tempfile.TemporaryDirectory(prefix="firmware-input-tests-")
        cls.binary = Path(cls.temp_dir.name) / "firmware_input_parsing_test"
        cjson_object = Path(cls.temp_dir.name) / "cJSON.o"
        subprocess.run(
            c_compiler
            + [
                "-I" + str(CJSON_INCLUDE),
                "-c",
                str(CJSON_INCLUDE / "cJSON.c"),
                "-o",
                str(cjson_object),
            ],
            check=True,
            cwd=ROOT,
        )
        subprocess.run(
            compiler
            + [
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                f"-I{ROOT / 'main'}",
                f"-I{CJSON_INCLUDE}",
                str(ROOT / "scripts/tests/firmware_input_parsing_test.cpp"),
                str(cjson_object),
                "-o",
                str(cls.binary),
            ],
            check=True,
            cwd=ROOT,
        )

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "temp_dir"):
            cls.temp_dir.cleanup()

    def test_firmware_input_parsing(self):
        subprocess.run([str(self.binary)], check=True, cwd=ROOT)


if __name__ == "__main__":
    unittest.main()
