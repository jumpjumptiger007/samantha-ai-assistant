// Adapted from nickvasilescu/hermes-desktop-os1's InfinityLoader at
// bea7d24aa03c1834b80390345c8875eaf9ae6502. See README.md for attribution.
import * as THREE from 'https://unpkg.com/three@0.182.0/build/three.module.js';

const CORAL = 0xd1684e;
const WHITE = 0xffffff;
const VIEWPORT_WIDTH = 240;
const VIEWPORT_HEIGHT = 320;
const REFERENCE_FPS = 60;
const DEFAULT_TRANSITION_SPEED = 0.45;
const DEFAULT_RING_MOTION = 0.08;
const EXPORT_MODE = window.__SAMANTHA_EXPORT_MODE__ === true;

const PRESETS = {
  idle: {
    label: 'Idle',
    description: 'Quiet, alive, and nearly resting.',
    rotation: 0.015,
    targetProgress: 0,
    ringMotion: 0.02,
  },
  listening: {
    label: 'Listening',
    description: 'The original open OS1 curve at its calm working pace.',
    rotation: 0.035,
    targetProgress: 0,
    ringMotion: 0.025,
  },
  thinking: {
    label: 'Thinking',
    description: 'The same curve partway through its helix-to-ring transformation.',
    rotation: 0.07,
    targetProgress: 0.7,
    ringMotion: 0.05,
  },
  speaking: {
    label: 'Speaking',
    description: 'The OS1 transformation nearly reaches its circular ring.',
    rotation: 0.12,
    targetProgress: 0.98,
    ringMotion: DEFAULT_RING_MOTION,
  },
};

function easeInOutQuad(t, b, c, d) {
  let eased = t / (d / 2);
  if (eased < 1) return (c / 2) * eased * eased + b;
  eased -= 1;
  return (-c / 2) * (eased * (eased - 2) - 1) + b;
}

// Geometry copied from the original loader: length 30, radius 5.6, and the
// same quarter-turn adjustment that produces its helix/infinity-like form.
class CustomSinCurve extends THREE.Curve {
  constructor(scale = 1) {
    super();
    this.scale = scale;
    this.length = 30;
    this.radius = 5.6;
  }

  getPoint(t, optionalTarget = new THREE.Vector3()) {
    const pi2 = Math.PI * 2;
    const x = this.length * Math.sin(pi2 * t);
    const y = this.radius * Math.cos(pi2 * 3 * t);

    let tValue = (t % 0.25) / 0.25;
    tValue = t % 0.25 - (2 * (1 - tValue) * tValue * -0.0185 + tValue * tValue * 0.25);
    if (Math.floor(t / 0.25) === 0 || Math.floor(t / 0.25) === 2) {
      tValue *= -1;
    }

    const z = this.radius * Math.sin(pi2 * 2 * (t - tValue));
    return optionalTarget.set(x, y, z).multiplyScalar(this.scale);
  }
}

class SamanthaPreview {
  constructor(canvas) {
    this.canvas = canvas;
    this.width = canvas.width || VIEWPORT_WIDTH;
    this.height = canvas.height || VIEWPORT_HEIGHT;
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(CORAL);
    this.group = new THREE.Group();
    this.scene.add(this.group);

    // The original camera distance keeps the open helix and transformed ring
    // readable in the smaller portrait Passport viewport.
    this.camera = new THREE.PerspectiveCamera(65, this.width / this.height, 1, 10000);
    this.camera.position.z = 150;

    const path = new CustomSinCurve(1);
    const geometry = new THREE.TubeGeometry(path, 200, 1.1, 2, true);
    const material = new THREE.MeshBasicMaterial({
      color: WHITE,
      transparent: true,
      opacity: 1,
      depthWrite: false,
    });
    this.mesh = new THREE.Mesh(geometry, material);
    this.group.add(this.mesh);

    const ringGeometry = new THREE.TorusGeometry(6.5, 0.4, 16, 32);
    const ringMaterial = new THREE.MeshBasicMaterial({
      color: WHITE,
      transparent: true,
      opacity: 0,
      depthWrite: false,
    });
    this.ring = new THREE.Mesh(ringGeometry, ringMaterial);
    this.ring.position.x = 31.1;
    this.ring.rotation.y = Math.PI / 2;
    this.group.add(this.ring);

    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: true,
      alpha: false,
      preserveDrawingBuffer: EXPORT_MODE,
    });
    this.renderer.setPixelRatio(1);
    this.renderer.setSize(this.width, this.height, false);
    this.renderer.setClearColor(CORAL, 1);

    this.activeState = 'idle';
    this.transitionProgress = 0;
    this.transitionTarget = 0;
    this.transitionStart = 0;
    this.transitionStartedAt = 0;
    this.transitionDuration = 0;
    this.transitionSpeed = DEFAULT_TRANSITION_SPEED;
    this.manualProgress = false;
    this.startTime = null;
    this.simulationFrame = 0;
    this.elapsedTime = 0;
    this.frame = this.frame.bind(this);
    if (!EXPORT_MODE) requestAnimationFrame(this.frame);
  }

  activePreset() {
    return PRESETS[this.activeState];
  }

  setViewport(width, height) {
    this.width = width;
    this.height = height;
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(width, height, false);
  }

  setState(state) {
    this.activeState = state;
    this.manualProgress = false;
    this.animateTo(this.activePreset().targetProgress);
  }

  animateTo(target) {
    this.transitionStart = this.transitionProgress;
    this.transitionTarget = target;
    this.transitionStartedAt = this.elapsedTime || 0;
    const directionSpeed = target < this.transitionStart
      ? this.transitionSpeed * 1.4
      : this.transitionSpeed;
    this.transitionDuration = Math.abs(target - this.transitionStart) / directionSpeed;
    if (this.transitionDuration === 0) this.transitionProgress = target;
  }

  setManualProgress(progress) {
    this.manualProgress = true;
    this.transitionProgress = progress;
    this.transitionStart = progress;
    this.transitionTarget = progress;
    this.transitionDuration = 0;
  }

  setTransitionSpeed(speed) {
    this.transitionSpeed = speed;
    if (!this.manualProgress && this.transitionProgress !== this.transitionTarget) {
      this.animateTo(this.transitionTarget);
    }
  }

  updateTransition(time) {
    if (this.manualProgress || this.transitionDuration === 0) return;

    const elapsed = Math.max(0, time - this.transitionStartedAt);
    const amount = THREE.MathUtils.clamp(elapsed / this.transitionDuration, 0, 1);
    this.transitionProgress = this.transitionStart
      + (this.transitionTarget - this.transitionStart) * amount;
    if (amount >= 1) this.transitionProgress = this.transitionTarget;
  }

  applyPose(time) {
    const preset = this.activePreset();
    const progress = THREE.MathUtils.clamp(this.transitionProgress, 0, 1);
    const animationProgress = easeInOutQuad(progress, 0, 1, 1);
    const orientationProgress = THREE.MathUtils.clamp(
      (animationProgress - 0.35) / 0.65,
      0,
      1,
    );
    const morphProgress = THREE.MathUtils.clamp(
      (animationProgress - 0.45) / 0.54,
      0,
      1,
    );
    const ringMotion = preset.ringMotion;

    // These transform values follow the upstream loader's tube compression,
    // half-length translation, group turn, and forward movement.
    this.mesh.scale.set(
      1 - morphProgress * 0.95,
      1 - morphProgress * 0.85,
      1 - morphProgress * 0.85,
    );
    this.mesh.position.x = morphProgress * 31.1 * 0.5;
    this.group.rotation.y = -Math.PI / 2 * orientationProgress;
    this.group.position.z = 50 * orientationProgress;

    // Crossfade with overlap so the reference tube remains legible as its ring
    // appears. The normalized slider maps continuously from open tube to ring.
    this.mesh.material.opacity = 1 - THREE.MathUtils.smoothstep(morphProgress, 0.62, 0.99);
    this.ring.material.opacity = THREE.MathUtils.smoothstep(morphProgress, 0.38, 0.83);

    // Keep the original ring's position, orientation, scale-up, and wobble
    // equations. The control scales the source wobble to a restrained amount.
    const baseRingScale = 0.1 + 0.9 * morphProgress;
    const wobbleTime = time * 10.8;
    const helixMomentum = this.mesh.rotation.x * 0.4;
    this.ring.rotation.x = Math.sin(wobbleTime * 1.3 + helixMomentum) * 2.5 * ringMotion;
    this.ring.rotation.y = Math.PI / 2 + Math.sin(wobbleTime * 0.7) * 1.8 * ringMotion;
    this.ring.rotation.z = Math.cos(wobbleTime * 0.9 + helixMomentum * 0.5) * 2.0 * ringMotion;
    this.ring.scale.set(
      baseRingScale * (1 + Math.sin(wobbleTime * 1.7) * 0.5 * ringMotion),
      baseRingScale * (1 + Math.cos(wobbleTime * 1.1) * 0.4 * ringMotion),
      baseRingScale * (1 + Math.sin(wobbleTime * 2.1) * 0.3 * ringMotion),
    );

  }

  renderAt(time) {
    const targetFrame = Math.max(
      this.simulationFrame,
      Math.floor(time * REFERENCE_FPS + 1e-7),
    );
    while (this.simulationFrame < targetFrame) {
      this.simulationFrame += 1;
      this.elapsedTime = this.simulationFrame / REFERENCE_FPS;
      this.updateTransition(this.elapsedTime);
      const progress = THREE.MathUtils.clamp(this.transitionProgress, 0, 1);
      const animationProgress = easeInOutQuad(progress, 0, 1, 1);
      this.mesh.rotation.x += this.activePreset().rotation + animationProgress;
    }

    this.applyPose(this.elapsedTime);
    this.renderer.render(this.scene, this.camera);
  }

  resetSimulation() {
    this.activeState = 'idle';
    this.transitionProgress = 0;
    this.transitionTarget = 0;
    this.transitionStart = 0;
    this.transitionStartedAt = 0;
    this.transitionDuration = 0;
    this.manualProgress = false;
    this.simulationFrame = 0;
    this.elapsedTime = 0;
    this.mesh.rotation.set(0, 0, 0);
    this.mesh.scale.set(1, 1, 1);
    this.mesh.position.set(0, 0, 0);
    this.renderAt(0);
  }

  readRgbaPixels() {
    const gl = this.renderer.getContext();
    const pixels = new Uint8Array(this.width * this.height * 4);
    gl.finish();
    gl.readPixels(0, 0, this.width, this.height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
    return pixels;
  }

  frame(timestamp) {
    if (this.startTime === null) this.startTime = timestamp;
    this.renderAt((timestamp - this.startTime) / 1000);
    requestAnimationFrame(this.frame);
  }
}

export const preview = new SamanthaPreview(document.querySelector('#preview'));
const description = document.querySelector('#state-description');
const controls = {
  rotation: document.querySelector('#rotation'),
  progress: document.querySelector('#progress'),
  transitionSpeed: document.querySelector('#transition-speed'),
  ringMotion: document.querySelector('#ring-motion'),
};
const outputs = {
  rotation: document.querySelector('#rotation-value'),
  progress: document.querySelector('#progress-value'),
  transitionSpeed: document.querySelector('#transition-speed-value'),
  ringMotion: document.querySelector('#ring-motion-value'),
};

function formatValue(name, value) {
  if (name === 'transitionSpeed') return `${value.toFixed(2)} progress/s`;
  return value.toFixed(3);
}

function syncControls() {
  const preset = preview.activePreset();
  description.textContent = preset.description;
  controls.rotation.value = preset.rotation;
  outputs.rotation.textContent = formatValue('rotation', preset.rotation);
  controls.progress.value = preview.transitionProgress;
  outputs.progress.textContent = formatValue('progress', preview.transitionProgress);
  controls.transitionSpeed.value = preview.transitionSpeed;
  outputs.transitionSpeed.textContent = formatValue('transitionSpeed', preview.transitionSpeed);
  controls.ringMotion.value = preset.ringMotion;
  outputs.ringMotion.textContent = formatValue('ringMotion', preset.ringMotion);
  document.querySelectorAll('[data-state]').forEach((button) => {
    button.setAttribute('aria-pressed', String(button.dataset.state === preview.activeState));
  });
}

controls.rotation.addEventListener('input', () => {
  const value = Number(controls.rotation.value);
  preview.activePreset().rotation = value;
  outputs.rotation.textContent = formatValue('rotation', value);
});

controls.progress.addEventListener('input', () => {
  const value = Number(controls.progress.value);
  preview.setManualProgress(value);
  outputs.progress.textContent = formatValue('progress', value);
  description.textContent = `${preview.activePreset().label}: manual transition.`;
});

controls.transitionSpeed.addEventListener('input', () => {
  const value = Number(controls.transitionSpeed.value);
  preview.setTransitionSpeed(value);
  outputs.transitionSpeed.textContent = formatValue('transitionSpeed', value);
});

controls.ringMotion.addEventListener('input', () => {
  const value = Number(controls.ringMotion.value);
  preview.activePreset().ringMotion = value;
  outputs.ringMotion.textContent = formatValue('ringMotion', value);
});

document.querySelectorAll('[data-state]').forEach((button) => {
  button.addEventListener('click', () => {
    preview.setState(button.dataset.state);
    syncControls();
  });
});

function syncProgressControl() {
  controls.progress.value = preview.transitionProgress;
  outputs.progress.textContent = formatValue('progress', preview.transitionProgress);
  requestAnimationFrame(syncProgressControl);
}

syncControls();
if (!EXPORT_MODE) syncProgressControl();
