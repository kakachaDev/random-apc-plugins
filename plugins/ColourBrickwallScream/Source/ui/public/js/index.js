import * as Juce from "./juce/index.js";

// ═══════════════════════════════════════════════════════
// KNOB RENDERING
// ═══════════════════════════════════════════════════════

const START  = (2 * Math.PI) / 3;   // 120° — 7 o'clock
const SWEEP  = (5 * Math.PI) / 3;   // 300° sweep
const CENTER = START + SWEEP * 0.5; // 270° — 12 o'clock (bipolar zero)

const C = {
  BZ_HI:   '#3C3330',
  BZ_LO:   '#141210',
  RIM:     '#2E2825',
  BD_HI:   '#2A2420',
  BD_LO:   '#111010',
  TRACK:   '#2A2220',
  ACCENT:  '#FF3C00',
  ACCENT2: '#FF6830',
  ACCD:    '#CC3000',
  DOT:     '#2E2825',
};

function drawKnob (canvas, norm) {
  const ctx  = canvas.getContext('2d');
  const w    = canvas.width;
  const h    = canvas.height;
  const cx   = w / 2;
  const cy   = h / 2;
  const big  = w >= 160;
  const r    = Math.min(w, h) / 2 - (big ? 8 : 6);
  const inR  = r * 0.70;
  const trR  = r * 0.83;
  const trW  = big ? 4 : 3;
  const bp   = canvas.dataset.bipolar === '1';
  const angle = START + norm * SWEEP;

  ctx.clearRect(0, 0, w, h);

  // High-drive glow (large knob only)
  if (big && norm > 0.78) {
    const alpha = Math.min(1, (norm - 0.78) * 4);
    ctx.beginPath();
    ctx.arc(cx, cy, r + 3, 0, Math.PI * 2);
    ctx.strokeStyle = `rgba(255,60,0,${alpha * 0.5})`;
    ctx.lineWidth = 4;
    ctx.stroke();
  }

  // Bezel
  const bzG = ctx.createRadialGradient(cx - r*0.28, cy - r*0.28, 0, cx, cy, r);
  bzG.addColorStop(0, C.BZ_HI);
  bzG.addColorStop(0.55, '#201A18');
  bzG.addColorStop(1, C.BZ_LO);
  ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.fillStyle = bzG; ctx.fill();

  ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.strokeStyle = C.RIM; ctx.lineWidth = 1.5; ctx.stroke();

  // Body
  const bdG = ctx.createRadialGradient(cx - inR*0.22, cy - inR*0.22, 0, cx, cy, inR);
  bdG.addColorStop(0, C.BD_HI);
  bdG.addColorStop(0.65, '#181412');
  bdG.addColorStop(1, C.BD_LO);
  ctx.beginPath(); ctx.arc(cx, cy, inR, 0, Math.PI * 2);
  ctx.fillStyle = bdG; ctx.fill();

  // Track background
  ctx.beginPath();
  ctx.arc(cx, cy, trR, START, START + SWEEP, false);
  ctx.strokeStyle = C.TRACK; ctx.lineWidth = trW; ctx.lineCap = 'round'; ctx.stroke();

  // Active arc
  const drawArc = (fromA, toA) => {
    if (Math.abs(toA - fromA) < 0.01) return;
    const arcG = ctx.createLinearGradient(
      cx + Math.cos(fromA)*trR, cy + Math.sin(fromA)*trR,
      cx + Math.cos(toA)*trR,   cy + Math.sin(toA)*trR);
    arcG.addColorStop(0, C.ACCD);
    arcG.addColorStop(1, C.ACCENT2);
    ctx.beginPath();
    ctx.arc(cx, cy, trR, fromA, toA, false);
    ctx.strokeStyle = arcG; ctx.lineWidth = trW; ctx.lineCap = 'round'; ctx.stroke();
  };

  if (bp) {
    if (norm > 0.5 + 0.01) drawArc(CENTER, angle);
    else if (norm < 0.5 - 0.01) drawArc(angle, CENTER);
  } else {
    if (norm > 0.005) drawArc(START, angle);
  }

  // Indicator line
  const iS = inR * 0.18, iE = inR * 0.70;
  ctx.beginPath();
  ctx.moveTo(cx + Math.cos(angle)*iS, cy + Math.sin(angle)*iS);
  ctx.lineTo(cx + Math.cos(angle)*iE, cy + Math.sin(angle)*iE);
  ctx.strokeStyle = C.ACCENT; ctx.lineWidth = big ? 2.5 : 2; ctx.lineCap = 'round'; ctx.stroke();

  // Tip dot
  ctx.beginPath();
  ctx.arc(cx + Math.cos(angle)*iE, cy + Math.sin(angle)*iE, big ? 3.5 : 2.5, 0, Math.PI*2);
  ctx.fillStyle = C.ACCENT2; ctx.fill();

  // Centre dot
  ctx.beginPath();
  ctx.arc(cx, cy, big ? 4 : 3, 0, Math.PI*2);
  ctx.fillStyle = C.DOT; ctx.fill();

  // Tick marks (large knob)
  if (big) {
    [[START, 0.6], [CENTER, 0.6], [START + SWEEP, 0.6]].forEach(([a]) => {
      ctx.beginPath();
      ctx.moveTo(cx + Math.cos(a)*(r-7), cy + Math.sin(a)*(r-7));
      ctx.lineTo(cx + Math.cos(a)*(r-2), cy + Math.sin(a)*(r-2));
      ctx.strokeStyle = 'rgba(136,112,104,0.6)'; ctx.lineWidth = 1; ctx.stroke();
    });
  }
}

// ═══════════════════════════════════════════════════════
// VALUE FORMATTING
// ═══════════════════════════════════════════════════════

const FMT = {
  'drive':             n => (n * 40).toFixed(1) + ' dB',
  'tone':              n => { const v = n*2-1; return (v>=0?'+':'')+v.toFixed(2); },
  'colour':            n => Math.round(n * 100) + '%',
  'attack_character':  n => { const ms = Math.exp(Math.log(0.1) + n * Math.log(500)); return ms.toFixed(1) + ' ms'; },
  'ceiling':           n => (-12 + n*12).toFixed(1) + ' dBFS',
  'mix':               n => Math.round(n * 100) + '%',
  'output_gain':       n => { const v = -12 + n*24; return (v>=0?'+':'')+v.toFixed(1)+' dB'; },
};

const VALUE_ID = {
  'drive': 'v-drive', 'tone': 'v-tone', 'colour': 'v-colour',
  'attack_character': 'v-attack', 'ceiling': 'v-ceiling',
  'mix': 'v-mix', 'output_gain': 'v-output',
};

// ═══════════════════════════════════════════════════════
// JUCE PARAMETER STATES
// ═══════════════════════════════════════════════════════

const paramStates = {};
const knobMap     = {};   // paramId → canvas element

function initJuceParams () {
  const paramIds = [
    'drive', 'character', 'tone', 'colour',
    'attack_character', 'ceiling', 'mix', 'output_gain'
  ];

  paramIds.forEach (id => {
    try {
      const state = Juce.getSliderState(id);
      paramStates[id] = state;

      state.valueChangedEvent.addListener(() => {
        const norm = state.getNormalisedValue();
        const canvas = knobMap[id];

        if (id === 'character') {
          const idx = Math.round(norm * 5);
          updateCharSelector(idx);
          return;
        }

        if (canvas) {
          drawKnob(canvas, norm);
          const el = document.getElementById(VALUE_ID[id]);
          if (el && FMT[id]) el.textContent = FMT[id](norm);
        }
      });
    } catch (e) {
      console.warn('Could not get JUCE state for param:', id, e);
    }
  });
}

// ═══════════════════════════════════════════════════════
// KNOB MOUSE INTERACTION
// ═══════════════════════════════════════════════════════

const localNorm = {};   // fallback when JUCE is not connected

function getNorm (id) {
  if (paramStates[id]) return paramStates[id].getNormalisedValue();
  return localNorm[id] ?? 0;
}

function setNorm (id, val) {
  const clamped = Math.max(0, Math.min(1, val));
  localNorm[id] = clamped;
  if (paramStates[id]) paramStates[id].setNormalisedValue(clamped);

  const canvas = knobMap[id];
  if (canvas) { drawKnob(canvas, clamped); }

  const el = document.getElementById(VALUE_ID[id]);
  if (el && FMT[id]) el.textContent = FMT[id](clamped);
}

function initKnobs () {
  document.querySelectorAll('canvas.knob').forEach(canvas => {
    const id = canvas.dataset.param;
    knobMap[id] = canvas;

    // Seed local norm from data-norm attribute
    const initNorm = parseFloat(canvas.dataset.norm);
    localNorm[id] = initNorm;
    drawKnob(canvas, initNorm);
    const el = document.getElementById(VALUE_ID[id]);
    if (el && FMT[id]) el.textContent = FMT[id](initNorm);

    let dragY = 0, dragN = 0, dragging = false;

    canvas.addEventListener('mousedown', e => {
      dragging = true;
      dragY    = e.clientY;
      dragN    = getNorm(id);
      document.body.style.cursor = 'ns-resize';
      e.preventDefault();
    });

    canvas.addEventListener('dblclick', () => {
      const def = parseFloat(canvas.dataset.default);
      const min = parseFloat(canvas.dataset.min);
      const max = parseFloat(canvas.dataset.max);
      setNorm(id, (def - min) / (max - min));
    });

    const onMove = e => {
      if (!dragging) return;
      const dy   = dragY - e.clientY;
      const sens = e.shiftKey ? 0.002 : 0.008;
      setNorm(id, dragN + dy * sens);
    };
    const onUp   = () => { dragging = false; document.body.style.cursor = ''; };

    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup',   onUp);

    canvas.addEventListener('wheel', e => {
      e.preventDefault();
      const d = -e.deltaY * (e.shiftKey ? 0.0005 : 0.004);
      setNorm(id, getNorm(id) + d);
    }, { passive: false });
  });
}

// ═══════════════════════════════════════════════════════
// CHARACTER SELECTOR
// ═══════════════════════════════════════════════════════

function updateCharSelector (idx) {
  document.querySelectorAll('.char-btn').forEach((b, i) =>
    b.classList.toggle('active', i === idx));
}

function initCharSelector () {
  document.querySelectorAll('.char-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const idx = parseInt(btn.dataset.char);
      setNorm('character', idx / 5);
      updateCharSelector(idx);
    });
  });
}

// ═══════════════════════════════════════════════════════
// METER RENDERING
// ═══════════════════════════════════════════════════════

function drawMeter (canvas, level, reversed) {
  const ctx  = canvas.getContext('2d');
  const w    = canvas.width;
  const h    = canvas.height;
  const segs = Math.floor(w / 5);
  const fill = Math.round(Math.max(0, Math.min(1, level)) * segs);

  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = '#0A0806';
  ctx.fillRect(0, 0, w, h);

  for (let i = 0; i < segs; i++) {
    const ratio = reversed ? (1 - i / segs) : (i / segs);
    const x     = reversed ? (w - (i+1)*5 + 1) : (i*5 + 1);
    const idx   = reversed ? (segs - 1 - i)     : i;
    const lit   = idx < fill;

    if      (ratio < 0.60) ctx.fillStyle = lit ? '#22CC22' : '#1A5C1A';
    else if (ratio < 0.75) ctx.fillStyle = lit ? '#88CC00' : '#2A5A10';
    else if (ratio < 0.85) ctx.fillStyle = lit ? '#FFAA00' : '#7A5500';
    else if (ratio < 0.95) ctx.fillStyle = lit ? '#FF4400' : '#8A2A00';
    else                   ctx.fillStyle = lit ? '#FF1100' : '#6A0000';

    ctx.fillRect(x, 1, 4, h - 2);
  }
}

function resizeMeters () {
  document.querySelectorAll('.m-canvas').forEach(c => {
    c.width = Math.max(1, Math.floor(c.getBoundingClientRect().width));
  });
}

// ── Meter update from JUCE timer (C++ calls window.__juceMeterUpdate) ────────
window.__juceMeterUpdate = function (data) {
  const cIn  = document.getElementById('m-in');
  const cGR  = document.getElementById('m-gr');
  const cOut = document.getElementById('m-out');

  if (cIn)  drawMeter(cIn,  data.input,  false);
  if (cGR)  drawMeter(cGR,  data.gr,     true);
  if (cOut) drawMeter(cOut, data.output, false);

  const toDb = v => (20 * Math.log10(Math.max(v, 0.001))).toFixed(1);
  const inEl  = document.getElementById('mv-in');
  const grEl  = document.getElementById('mv-gr');
  const outEl = document.getElementById('mv-out');

  if (inEl)  inEl.textContent  = toDb(data.input)   + ' dB';
  if (grEl)  grEl.textContent  = '-' + Math.abs(parseFloat(toDb(1 - data.gr * 0.6))) + ' dB';
  if (outEl) outEl.textContent = toDb(data.output)  + ' dB';
};

// ═══════════════════════════════════════════════════════
// INIT
// ═══════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════
// SCALE TO WINDOW
// ═══════════════════════════════════════════════════════

const DESIGN_W = 700;

function applyScale () {
  const scale = window.innerWidth / DESIGN_W;   // aspect is locked by C++, so W is enough
  const root  = document.getElementById('root');
  if (root) root.style.transform = `scale(${scale})`;
}

window.addEventListener('resize', applyScale);

// ═══════════════════════════════════════════════════════
// INIT
// ═══════════════════════════════════════════════════════

document.addEventListener('DOMContentLoaded', () => {
  applyScale();
  resizeMeters();
  initKnobs();
  initCharSelector();
  initJuceParams();
});
