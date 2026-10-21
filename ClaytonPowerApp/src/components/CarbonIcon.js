import React from 'react';
import Svg, { Path, Circle, Rect } from 'react-native-svg';
import { colors } from '../utils/theme';

// Carbon Blue pictograms (spec §4): stroke-based, 24 px viewBox, 1.8 px
// stroke, round caps, never filled. The first block is copied verbatim from
// the display design spec; the rest are app actions drawn in the same style.
const ICONS = {
  plug: (
    <>
      <Path d="M9 2.5v5M15 2.5v5" />
      <Path d="M7 7.5h10v3.5a5 5 0 0 1-10 0z" />
      <Path d="M12 16v3" />
      <Path d="M8.5 21.5c2 .8 5 .8 7 0" />
    </>
  ),
  car: (
    <>
      <Path d="M3.5 16.5v-3l1.8-4.6A1.6 1.6 0 0 1 6.8 8h10.4a1.6 1.6 0 0 1 1.5.9l1.8 4.6v3" />
      <Path d="M3.5 13.5h17" />
      <Circle cx="7" cy="16.5" r="1.7" />
      <Circle cx="17" cy="16.5" r="1.7" />
    </>
  ),
  sun: (
    <>
      <Circle cx="12" cy="12" r="4" />
      <Path d="M12 2.5v2.5M12 19v2.5M2.5 12H5M19 12h2.5M5.3 5.3l1.8 1.8M16.9 16.9l1.8 1.8M18.7 5.3l-1.8 1.8M7.1 16.9l-1.8 1.8" />
    </>
  ),
  socket: (
    <>
      <Rect x="3.5" y="3.5" width="17" height="17" rx="2" />
      <Path d="M9.5 10v4M14.5 10v4" />
    </>
  ),
  dc: (
    <>
      <Path d="M4.5 10h15" />
      <Path d="M4.5 14.5h3.4M10.3 14.5h3.4M16.1 14.5h3.4" />
    </>
  ),
  warn: (
    <>
      <Path d="M12 3.5 21.5 20h-19z" />
      <Path d="M12 10v4.5" />
      <Path d="M12 17.6v.2" />
    </>
  ),
  bt: <Path d="M6.5 7 17.5 17 12 21.5V2.5L17.5 7 6.5 17" />,
  gear: (
    <>
      <Circle cx="12" cy="12" r="3.4" />
      <Path d="M12 3v3M12 18v3M3 12h3M18 12h3M5.6 5.6l2.1 2.1M16.3 16.3l2.1 2.1M18.4 5.6l-2.1 2.1M7.7 16.3l-2.1 2.1" />
    </>
  ),
  batt: (
    <>
      <Rect x="2.5" y="8" width="16" height="8" rx="1.5" />
      <Path d="M21.5 10.5v3" />
      <Path d="M6 10.5v3M9.5 10.5v3" />
    </>
  ),
  eye: (
    <>
      <Path d="M2.5 12s3.5-6 9.5-6 9.5 6 9.5 6-3.5 6-9.5 6-9.5-6-9.5-6z" />
      <Circle cx="12" cy="12" r="2.6" />
    </>
  ),
  therm: (
    <>
      <Circle cx="10.5" cy="17.5" r="3.5" />
      <Path d="M8.5 15V5.5a2 2 0 0 1 4 0V15" />
      <Path d="M15.5 7h4M15.5 10.5h4" />
    </>
  ),
  chev: <Path d="M9 5.5 15.5 12 9 18.5" />,

  // App actions (same drawing rules)
  chevDown: <Path d="M5.5 9 12 15.5 18.5 9" />,
  back: <Path d="M15 5.5 8.5 12 15 18.5" />,
  close: <Path d="M6 6l12 12M18 6 6 18" />,
  check: <Path d="M4.5 12.5 9.5 17.5 19.5 6.5" />,
  plus: <Path d="M12 5v14M5 12h14" />,
  minus: <Path d="M5 12h14" />,
  dash: (
    <>
      <Rect x="3.5" y="3.5" width="7" height="9" />
      <Rect x="13.5" y="3.5" width="7" height="5" />
      <Rect x="3.5" y="15.5" width="7" height="5" />
      <Rect x="13.5" y="11.5" width="7" height="9" />
    </>
  ),
  update: (
    <>
      <Path d="M12 3.5v11" />
      <Path d="M7.5 10 12 14.5 16.5 10" />
      <Path d="M4 15.5v4h16v-4" />
    </>
  ),
  refresh: (
    <>
      <Path d="M19.5 12a7.5 7.5 0 1 1-2.2-5.3" />
      <Path d="M19.5 4v4h-4" />
    </>
  ),
  scan: (
    <>
      <Circle cx="12" cy="12" r="1.6" />
      <Path d="M8.2 15.8a5.4 5.4 0 0 1 0-7.6M15.8 8.2a5.4 5.4 0 0 1 0 7.6" />
      <Path d="M5.3 18.7a9.5 9.5 0 0 1 0-13.4M18.7 5.3a9.5 9.5 0 0 1 0 13.4" />
    </>
  ),
  link: (
    <>
      <Path d="M10 14a4 4 0 0 0 5.7 0l3.3-3.3a4 4 0 0 0-5.7-5.7l-1.2 1.2" />
      <Path d="M14 10a4 4 0 0 0-5.7 0L5 13.3a4 4 0 0 0 5.7 5.7l1.2-1.2" />
    </>
  ),
  lock: (
    <>
      <Rect x="5" y="10.5" width="14" height="10" />
      <Path d="M8 10.5V7.5a4 4 0 0 1 8 0v3" />
    </>
  ),
  stop: <Path d="M6.5 6.5h11v11h-11z" />,
  input: (
    <>
      <Path d="M3.5 12h11" />
      <Path d="M10.5 7.5 15 12l-4.5 4.5" />
      <Path d="M15 3.5h5.5v17H15" />
    </>
  ),
};

export default function CarbonIcon({ name, size = 24, color = colors.ink, strokeWidth = 1.8 }) {
  const glyph = ICONS[name];
  if (!glyph) return null;
  return (
    <Svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke={color}
      strokeWidth={strokeWidth}
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      {glyph}
    </Svg>
  );
}
