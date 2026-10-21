import React, { useState } from 'react';
import { View } from 'react-native';
import Svg, { Circle, Defs, Line, LinearGradient, Path, Stop, Text as SvgText } from 'react-native-svg';
import { colors, font } from '../utils/theme';
import { CHART_PTS, NOW_IDX, WINDOW_HOURS } from '../services/socHistory';

const PAD = { left: 24, right: 10, top: 10, bottom: 24 };

// Time axis: -H/2 -H/4 NOW +H/4 +H/2 (firmware update_time_axis()).
function axisLabels() {
  const stepMin = WINDOW_HOURS * 15;
  return [0, 1, 2, 3, 4].map((i) => {
    const off = (i - 2) * stepMin;
    if (off === 0) return 'NOW';
    const sign = off > 0 ? '+' : '-';
    const abs = Math.abs(off);
    return abs % 60 === 0 ? `${sign}${abs / 60}h` : `${sign}${abs}m`;
  });
}

/**
 * Forecast chart (spec §6.1): time-symmetric axis with NOW in the middle,
 * history to the left and a time-true dashed projection to the right.
 */
export default function ForecastChart({ history, soc, charging, discharging, minutesLeft, style }) {
  // Fills whatever space the parent gives it (flex), so the dashboard can fit
  // one screen on any phone height.
  const [size, setSize] = useState({ width: 0, height: 0 });
  const { width, height } = size;
  const labels = axisLabels();

  const x0 = PAD.left;
  const y0 = PAD.top;
  const w = Math.max(1, width - PAD.left - PAD.right);
  const h = height - PAD.top - PAD.bottom;
  const cx = (idx) => x0 + (idx * w) / (CHART_PTS - 1);
  const cy = (v) => y0 + ((100 - Math.max(0, Math.min(100, v))) * h) / 100;

  let histLine = '';
  let histFill = '';
  const firstIdx = history.findIndex((v) => v != null);
  if (firstIdx >= 0) {
    const pts = [];
    for (let i = firstIdx; i <= NOW_IDX; i++) pts.push(`${cx(i).toFixed(1)} ${cy(history[i]).toFixed(1)}`);
    if (pts.length === 1) pts.unshift(`${(cx(NOW_IDX) - 0.5).toFixed(1)} ${cy(history[NOW_IDX]).toFixed(1)}`);
    histLine = `M ${pts.join(' L ')}`;
    histFill = `${histLine} L ${cx(NOW_IDX).toFixed(1)} ${y0 + h} L ${cx(firstIdx).toFixed(1)} ${y0 + h} Z`;
  }

  // Projection: reaches the target where the remaining time lands, then flat.
  const target = charging ? 100 : discharging ? 0 : soc;
  const halfMin = WINDOW_HOURS * 30;
  const nx = cx(NOW_IDX);
  const rx = cx(CHART_PTS - 1);
  let projection = `M ${nx} ${cy(soc)} L ${rx} ${cy(soc)}`;
  let ring = null;
  if ((charging || discharging) && minutesLeft > 0) {
    if (minutesLeft <= halfMin) {
      const kx = nx + ((rx - nx) * minutesLeft) / halfMin;
      projection = `M ${nx} ${cy(soc)} L ${kx} ${cy(target)} L ${rx} ${cy(target)}`;
      ring = { x: kx, y: cy(target) };
    } else {
      const edge = soc + ((target - soc) * halfMin) / minutesLeft;
      projection = `M ${nx} ${cy(soc)} L ${rx} ${cy(edge)}`;
    }
  }

  return (
    <View
      style={style}
      onLayout={(e) => setSize({ width: e.nativeEvent.layout.width, height: e.nativeEvent.layout.height })}
    >
      {width > 0 && height > PAD.top + PAD.bottom && (
        <Svg width={width} height={height}>
          <Defs>
            <LinearGradient id="histFill" x1="0" y1="0" x2="0" y2="1">
              <Stop offset="0" stopColor={colors.blue} stopOpacity={0.22} />
              <Stop offset="1" stopColor={colors.blue} stopOpacity={0} />
            </LinearGradient>
          </Defs>

          {[25, 50, 75, 100].map((v) => (
            <React.Fragment key={v}>
              <Line x1={x0} y1={cy(v)} x2={x0 + w} y2={cy(v)} stroke={colors.track} strokeWidth={1} />
              <SvgText x={x0 - 6} y={cy(v) + 3.5} fill={colors.faint} fontSize={10} fontFamily={font.semibold} textAnchor="end">
                {v}
              </SvgText>
            </React.Fragment>
          ))}

          <Line x1={nx} y1={y0 - 4} x2={nx} y2={y0 + h + 4} stroke={colors.line} strokeWidth={1} />

          {!!histFill && <Path d={histFill} fill="url(#histFill)" />}
          {!!histLine && (
            <Path d={histLine} stroke={colors.blue} strokeWidth={2.5} fill="none" strokeLinejoin="round" strokeLinecap="round" />
          )}

          <Path d={projection} stroke={colors.blue} strokeWidth={2} strokeDasharray="5 6" fill="none" opacity={0.65} />
          {ring && <Circle cx={ring.x} cy={ring.y} r={4.5} stroke={colors.blue} strokeWidth={2} fill={colors.bg} />}

          <Circle cx={nx} cy={cy(soc)} r={5} fill={colors.ink} />

          {labels.map((label, i) => (
            <SvgText
              key={label}
              x={x0 + (i * w) / 4}
              y={height - 6}
              fill={i === 2 ? colors.dim : colors.faint}
              fontSize={10}
              fontFamily={i === 2 ? font.bold : font.semibold}
              textAnchor={i === 0 ? 'start' : i === 4 ? 'end' : 'middle'}
            >
              {label}
            </SvgText>
          ))}
        </Svg>
      )}
    </View>
  );
}
