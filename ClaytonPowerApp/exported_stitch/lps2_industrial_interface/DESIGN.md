---
name: LPS2 Industrial Interface
colors:
  surface: '#131313'
  surface-dim: '#131313'
  surface-bright: '#393939'
  surface-container-lowest: '#0e0e0e'
  surface-container-low: '#1c1b1b'
  surface-container: '#201f1f'
  surface-container-high: '#2a2a2a'
  surface-container-highest: '#353534'
  on-surface: '#e5e2e1'
  on-surface-variant: '#c1c6d7'
  inverse-surface: '#e5e2e1'
  inverse-on-surface: '#313030'
  outline: '#8b90a0'
  outline-variant: '#414755'
  surface-tint: '#adc6ff'
  primary: '#adc6ff'
  on-primary: '#002e69'
  primary-container: '#4b8eff'
  on-primary-container: '#00285c'
  inverse-primary: '#005bc1'
  secondary: '#c8c6c8'
  on-secondary: '#303032'
  secondary-container: '#474649'
  on-secondary-container: '#b6b4b7'
  tertiary: '#ffb595'
  on-tertiary: '#571e00'
  tertiary-container: '#ef6719'
  on-tertiary-container: '#4c1a00'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#d8e2ff'
  primary-fixed-dim: '#adc6ff'
  on-primary-fixed: '#001a41'
  on-primary-fixed-variant: '#004493'
  secondary-fixed: '#e4e2e4'
  secondary-fixed-dim: '#c8c6c8'
  on-secondary-fixed: '#1b1b1d'
  on-secondary-fixed-variant: '#474649'
  tertiary-fixed: '#ffdbcc'
  tertiary-fixed-dim: '#ffb595'
  on-tertiary-fixed: '#351000'
  on-tertiary-fixed-variant: '#7c2e00'
  background: '#131313'
  on-background: '#e5e2e1'
  surface-variant: '#353534'
typography:
  display-lg:
    fontFamily: Inter
    fontSize: 48px
    fontWeight: '700'
    lineHeight: '1.1'
    letterSpacing: -0.02em
  headline-md:
    fontFamily: Inter
    fontSize: 24px
    fontWeight: '600'
    lineHeight: '1.3'
    letterSpacing: -0.01em
  body-base:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '400'
    lineHeight: '1.5'
    letterSpacing: '0'
  label-sm:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '600'
    lineHeight: '1'
    letterSpacing: 0.05em
  data-tabular:
    fontFamily: Inter
    fontSize: 18px
    fontWeight: '500'
    lineHeight: '1'
rounded:
  sm: 0.25rem
  DEFAULT: 0.5rem
  md: 0.75rem
  lg: 1rem
  xl: 1.5rem
  full: 9999px
spacing:
  unit: 4px
  xs: 4px
  sm: 8px
  md: 16px
  lg: 24px
  xl: 32px
  container-padding: 20px
  card-gap: 12px
---

## Brand & Style
The design system is engineered for the Clayton Power LPS2, bridging the gap between rugged industrial hardware and high-end consumer electronics. The brand personality is authoritative, reliable, and precise, drawing heavily from Scandinavian industrial design principles. It evokes a sense of "technical calm"—where complex power data is presented through a lens of extreme clarity.

The aesthetic follows a **High-Contrast Minimalism** style. It prioritizes functional density and immediate legibility, ensuring that users in high-stakes or outdoor environments can parse critical battery and output data at a glance. The interface avoids decorative flourishes, relying on mathematical spacing and structural card layouts to create a professional, "tool-like" experience.

## Colors
This design system utilizes a deep-black dark mode to maximize contrast and reduce power consumption on mobile displays. The palette is strictly functional:

- **Primary Blue (#007AFF):** Reserved for active states, primary actions, and current flow indicators.
- **Surface Dark (#121212):** The base canvas color, providing a true-black foundation.
- **Surface Elevated (#1C1C1E):** A slightly lighter grey for card backgrounds to create depth without borders.
- **Semantic Colors:** Green (Charging/Healthy), Amber (Warning/Low Capacity), and Red (Fault/Critical) are used for status-driven data visualization.
- **Typography:** Pure white (#FFFFFF) is used for data and headers, while muted grey (#A1A1AA) is used for secondary labels and units.

## Typography
Inter is the sole typeface, chosen for its utilitarian clarity and excellent rendering in low-light environments. 

The typographic hierarchy distinguishes between **Status Data** (large, bold, high-visibility) and **Metadata** (small, uppercase labels). Tabular numerals must be enabled for all power readings (Watts, Volts, Percentages) to prevent layout shifting as values fluctuate. Headers are compact with slight negative letter-spacing to reinforce the industrial, engineered feel.

## Layout & Spacing
The layout uses a **Fluid Grid** system optimized for mobile and tablet monitoring displays. A 4px baseline grid governs all spacing to maintain mathematical rigor.

Primary navigation and system-wide stats are pinned to the top or bottom, while the core content resides in a single or double-column stack of cards. Gutters are kept tight (12px) to maximize the "dashboard" feel, ensuring that as much technical information as possible is visible without scrolling, while maintaining enough white space to avoid visual fatigue.

## Elevation & Depth
Depth is achieved through **Tonal Layers** rather than heavy shadows or borders. The hierarchy is established by stacking lighter grey surfaces on the absolute black background.

- **Level 0 (Background):** #121212.
- **Level 1 (Cards):** #1C1C1E.
- **Level 2 (Modals/Overlays):** #2C2C2E with a very subtle, diffused 20px black shadow (0px 4px 20px rgba(0,0,0,0.5)).

Borders are eliminated entirely to create a seamless "Glass-to-Edge" look, common in Scandinavian digital interfaces. High-contrast separations are handled by the value change between the background and card surfaces.

## Shapes
In line with the 8px requirement, this design system uses a consistent **Rounded (0.5rem)** radius for all primary containers and buttons. 

- **Cards & Inputs:** 8px (0.5rem).
- **Secondary Chips/Tags:** 4px (0.25rem) to distinguish them from primary interactive elements.
- **Progress Bars:** Fully rounded (pill) for "Inner fill" components to suggest fluid movement of energy.

This radius provides a "soft-tech" feel that balances the sharp, industrial nature of the dark color palette.

## Components
- **Data Cards:** The primary vessel for information. Use #1C1C1E backgrounds. Icons are placed in the top-left, while primary metrics (e.g., "1200W") are centered or right-aligned in large bold weights.
- **Control Buttons:** High-contrast #007AFF backgrounds with white text for primary actions. Ghost buttons (white outline, no fill) for secondary system settings.
- **Status Indicators:** Small, circular pips using semantic colors (Green/Amber/Red) placed next to component labels (e.g., "AC Output • Active").
- **Energy Flow Gauges:** Thin, horizontal progress bars or circular rings with 4px stroke widths. Use the Primary Blue for active discharge and Semantic Green for charging states.
- **Segmented Controllers:** Used for switching between "Input," "Output," and "System" views. These should appear as flat, recessed tabs within a single container.
- **Input Fields:** Dark grey backgrounds with 8px rounding. Focus states are indicated by a 2px #007AFF outer glow, never a change in border color.