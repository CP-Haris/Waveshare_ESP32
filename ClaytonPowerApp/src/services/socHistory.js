// SoC history ring buffer for the forecast chart (spec §6.1) — port of the
// firmware's update_chart() sampling. One buffer per unit; lives as long as
// the app process.

export const CHART_PTS = 61;
export const NOW_IDX = 30;
export const WINDOW_HOURS = 2;
const SAMPLE_MS = (WINDOW_HOURS * 3600000) / (2 * NOW_IDX);

const buffers = new Map();

/**
 * Feed the latest SoC for a unit. Returns the history slots 0..NOW_IDX,
 * right-aligned against "now"; slots without a real sample are null.
 */
export function recordSoc(unitKey, soc, now = Date.now()) {
  let buf = buffers.get(unitKey);
  if (!buf) {
    buf = { hist: new Array(NOW_IDX + 1).fill(null), count: 1, last: now };
    buf.hist[NOW_IDX] = soc;
    buffers.set(unitKey, buf);
  } else if (now - buf.last >= SAMPLE_MS) {
    buf.hist.shift();
    buf.hist.push(soc);
    if (buf.count <= NOW_IDX) buf.count += 1;
    buf.last = now;
  } else {
    buf.hist[NOW_IDX] = soc; // live head
  }
  return buf.hist.slice();
}
