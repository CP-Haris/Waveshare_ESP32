// Carbon Blue tokens — see Docs/Carbon Blue App-spec.md §2-3.
// Values are shared 1:1 with the display firmware (dashboard_carbon.c CB_*):
// near-black ground, grey plates for everything you can press.

export const colors = {
  bg: '#0B0C0E',
  panel: '#26292E',
  panelPressed: '#34383F',
  sheet: '#1A1C20',
  line: '#26282C',
  rowLine: '#1A1C20',
  edge: '#3E434A',
  track: '#1F2125',

  ink: '#EFEDE8',
  soft: '#B9BCB6',
  dim: '#82868C',
  faint: '#4A4E54',

  blue: '#4E9EEB',
  onBlue: '#0E1A28',
  yellow: '#E6C84A',
  red: '#E25454',

  scrim: 'rgba(5,6,8,0.62)',
};

export const font = {
  semibold: 'Barlow-SemiBold',
  bold: 'Barlow-Bold',
};

export const fontAssets = {
  [font.semibold]: require('../../assets/fonts/BarlowSemiCondensed-SemiBold.ttf'),
  [font.bold]: require('../../assets/fonts/BarlowSemiCondensed-Bold.ttf'),
};

const num = { fontVariant: ['tabular-nums'] };

// Typography presets (spec §3). Custom fonts carry their own weight, so no
// fontWeight is set — Android would otherwise fall back to the system font.
export const type = {
  soc: { fontFamily: font.bold, fontSize: 84, lineHeight: 84, color: colors.ink, ...num },
  total: { fontFamily: font.bold, fontSize: 42, lineHeight: 44, color: colors.ink, ...num },
  value: { fontFamily: font.bold, fontSize: 22, lineHeight: 26, color: colors.ink, ...num },
  title: { fontFamily: font.bold, fontSize: 20, letterSpacing: 4, color: colors.ink },
  zone: { fontFamily: font.bold, fontSize: 15, letterSpacing: 3, color: colors.ink },
  section: { fontFamily: font.bold, fontSize: 13, letterSpacing: 2, color: colors.dim },
  label: { fontFamily: font.semibold, fontSize: 18, color: colors.ink },
  body: { fontFamily: font.semibold, fontSize: 15, lineHeight: 21, color: colors.soft },
  small: { fontFamily: font.semibold, fontSize: 13, color: colors.dim, ...num },
  micro: { fontFamily: font.semibold, fontSize: 11, letterSpacing: 1, color: colors.faint, ...num },
  button: { fontFamily: font.bold, fontSize: 15, letterSpacing: 2, color: colors.ink },
};

export const spacing = {
  xs: 4,
  sm: 8,
  md: 16,
  lg: 24,
  xl: 32,
};
