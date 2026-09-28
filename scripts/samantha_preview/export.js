import { preview } from './preview.js';

const FPS = 20;
const FRAME_SECONDS = 1 / FPS;
const OUTPUTS = [
  { name: 'assets/samantha/idle.gif', width: 128, height: 128, loop: true },
  { name: 'assets/samantha/listening.gif', width: 128, height: 128, loop: true },
  { name: 'assets/samantha/thinking.gif', width: 128, height: 128, loop: false },
  { name: 'assets/samantha/speaking.gif', width: 128, height: 128, loop: true },
  { name: 'docs/samantha-ui-demo.gif', width: 240, height: 320, loop: true },
];

const button = document.querySelector('#export-all');
const status = document.querySelector('#status');
const results = document.querySelector('#results');
const previews = document.querySelector('#previews');

function yieldToBrowser() {
  return new Promise((resolve) => setTimeout(resolve, 0));
}

async function collectFrames(width, height, count, renderFrame, label) {
  preview.setViewport(width, height);
  const frames = [];
  for (let index = 0; index < count; index += 1) {
    renderFrame(index);
    frames.push(preview.readRgbaPixels());
    if (index % 12 === 0) {
      status.textContent = `Rendering ${label}: frame ${index + 1} of ${count}…`;
      await yieldToBrowser();
    }
  }
  return frames;
}

async function uploadGif(output, frames) {
  const frameBytes = output.width * output.height * 4;
  const pixels = new Uint8Array(frameBytes * frames.length);
  frames.forEach((frame, index) => pixels.set(frame, index * frameBytes));
  const response = await fetch('/__export', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/octet-stream',
      'X-Asset-Name': output.name,
      'X-Frame-Width': String(output.width),
      'X-Frame-Height': String(output.height),
      'X-Frame-Count': String(frames.length),
      'X-Frame-Rate': String(FPS),
      'X-Loop': String(output.loop),
    },
    body: pixels,
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || `Export failed for ${output.name}`);
  const item = document.createElement('li');
  item.textContent = `${result.path}: ${result.frames} frames, ${result.bytes.toLocaleString()} bytes`;
  results.append(item);
  const figure = document.createElement('figure');
  const image = document.createElement('img');
  image.src = `/${result.path}?generated=${Date.now()}`;
  image.alt = result.path;
  const caption = document.createElement('figcaption');
  caption.textContent = result.path;
  figure.append(image, caption);
  previews.append(figure);
  return result;
}

async function exportLoop(output, frameCount, prepare) {
  preview.setViewport(output.width, output.height);
  preview.resetSimulation();
  if (prepare) prepare();
  const frames = await collectFrames(
    output.width,
    output.height,
    frameCount,
    (index) => preview.renderAt(index * FRAME_SECONDS),
    output.name,
  );
  status.textContent = `Encoding ${output.name}…`;
  return uploadGif(output, frames);
}

async function exportSpeaking(output) {
  preview.setViewport(output.width, output.height);
  preview.resetSimulation();
  preview.setManualProgress(0.98);
  preview.setState('speaking');
  const forward = await collectFrames(
    output.width,
    output.height,
    40,
    (index) => preview.renderAt(index * FRAME_SECONDS),
    output.name,
  );
  const frames = forward.concat(forward.slice(1, -1).reverse());
  status.textContent = `Encoding ${output.name}…`;
  return uploadGif(output, frames);
}

async function exportThinking(output) {
  preview.setViewport(output.width, output.height);
  preview.resetSimulation();
  preview.setState('thinking');
  const frames = [];
  let index = 0;
  while (true) {
    preview.renderAt(index * FRAME_SECONDS);
    frames.push(preview.readRgbaPixels());
    if (preview.transitionProgress >= preview.transitionTarget - 1e-7) break;
    index += 1;
    if (index % 12 === 0) {
      status.textContent = `Rendering ${output.name}: frame ${index}…`;
      await yieldToBrowser();
    }
    if (index > 200) throw new Error('Thinking transition exceeded its expected duration');
  }
  preview.renderAt(index * FRAME_SECONDS);
  frames.push(preview.readRgbaPixels());
  status.textContent = `Encoding ${output.name}…`;
  return uploadGif(output, frames);
}

async function exportShowcase(output) {
  preview.setViewport(output.width, output.height);
  preview.resetSimulation();
  preview.setState('listening');
  const frames = [];
  let frameIndex = 0;

  const pushNext = async () => {
    preview.renderAt(frameIndex * FRAME_SECONDS);
    frames.push(preview.readRgbaPixels());
    frameIndex += 1;
    if (frameIndex % 12 === 0) {
      status.textContent = `Rendering README showcase: frame ${frameIndex}…`;
      await yieldToBrowser();
    }
  };

  for (let index = 0; index < 20; index += 1) await pushNext();

  preview.setState('speaking');
  while (preview.transitionProgress < preview.transitionTarget - 1e-7) {
    await pushNext();
    if (frameIndex > 200) throw new Error('Showcase forward morph exceeded its expected duration');
  }
  for (let index = 0; index < 24; index += 1) await pushNext();

  preview.setState('listening');
  while (preview.transitionProgress > 1e-7) {
    await pushNext();
    if (frameIndex > 400) throw new Error('Showcase reverse morph exceeded its expected duration');
  }

  // Finish on the same open-helix phase as frame zero to close the loop cleanly.
  const angle = preview.mesh.rotation.x;
  let closingFrames = 0;
  let smallestPhaseError = Math.abs(Math.atan2(Math.sin(angle), Math.cos(angle)));
  for (let candidate = 1; candidate <= 80; candidate += 1) {
    const nextAngle = angle + candidate * 3 * 0.035;
    const phaseError = Math.abs(Math.atan2(Math.sin(nextAngle), Math.cos(nextAngle)));
    if (phaseError < smallestPhaseError) {
      smallestPhaseError = phaseError;
      closingFrames = candidate;
    }
  }
  for (let index = 0; index < closingFrames; index += 1) await pushNext();

  status.textContent = 'Encoding README showcase…';
  return uploadGif(output, frames);
}

button.addEventListener('click', async () => {
  button.disabled = true;
  results.replaceChildren();
  previews.replaceChildren();
  try {
    for (const output of OUTPUTS) {
      if (output.name.endsWith('/idle.gif')) {
        await exportLoop(output, 140);
      } else if (output.name.endsWith('/listening.gif')) {
        await exportLoop(output, 60, () => preview.setState('listening'));
      } else if (output.name.endsWith('/thinking.gif')) {
        await exportThinking(output);
      } else if (output.name.endsWith('/speaking.gif')) {
        await exportSpeaking(output);
      } else {
        await exportShowcase(output);
      }
    }
    status.textContent = 'All five GIFs exported successfully.';
  } catch (error) {
    status.textContent = `Export stopped: ${error.message}`;
    console.error(error);
  } finally {
    button.disabled = false;
  }
});
