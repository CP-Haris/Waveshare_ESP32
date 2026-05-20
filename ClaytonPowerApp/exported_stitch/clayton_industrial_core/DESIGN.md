---
name: Clayton Industrial Core
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
  secondary: '#4ae183'
  on-secondary: '#003919'
  secondary-container: '#06bb63'
  on-secondary-container: '#00431f'
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
  secondary-fixed: '#6bfe9c'
  secondary-fixed-dim: '#4ae183'
  on-secondary-fixed: '#00210c'
  on-secondary-fixed-variant: '#005228'
  tertiary-fixed: '#ffdbcc'
  tertiary-fixed-dim: '#ffb595'
  on-tertiary-fixed: '#351000'
  on-tertiary-fixed-variant: '#7c2e00'
  background: '#131313'
  on-background: '#e5e2e1'
  surface-variant: '#353534'
typography:
  display-data:
    fontFamily: Inter
    fontSize: 48px
    fontWeight: '700'
    lineHeight: 56px
    letterSpacing: -0.02em
  headline-lg:
    fontFamily: Inter
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
  headline-md:
    fontFamily: Inter
    fontSize: 20px
    fontWeight: '600'
    lineHeight: 28px
  body-lg:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '400'
    lineHeight: 24px
  body-md:
    fontFamily: Inter
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  label-caps:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '700'
    lineHeight: 16px
    letterSpacing: 0.05em
  numeric-md:
    fontFamily: Inter
    fontSize: 18px
    fontWeight: '500'
    lineHeight: 24px
rounded:
  sm: 0.25rem
  DEFAULT: 0.5rem
  md: 0.75rem
  lg: 1rem
  xl: 1.5rem
  full: 9999px
spacing:
  unit: 8px
  container-padding: 24px
  stack-sm: 8px
  stack-md: 16px
  stack-lg: 32px
  gutter: 16px
---

## Brand & Style

The visual identity of this design system is rooted in the Scandinavian principles of functionalism, precision, and restrained elegance. It is designed for mission-critical industrial energy management, where clarity is a safety requirement and high-trust aesthetics are paramount.

The style is **Corporate / Modern** with a **Technical** overlay. It utilizes a deep, monochromatic foundation to highlight critical data points and system statuses. The interface should feel like a high-fidelity instrument panel: robust, responsive, and authoritative. By balancing clean whitespace (even in dark mode) with precise technical detailing, the design system conveys the reliability of Clayton Power hardware.

## Colors

The palette is anchored by a "Obsidian & Slate" foundation. The background utilizes a deep charcoal (`#121212`) rather than pure black to maintain depth and reduce eye strain in industrial environments. 

- **Technical Blue:** Reserved for primary actions, active connection states, and brand-aligned accents.
- **Vibrant Green:** Indicates "Healthy" or "Active" states, providing a high-contrast signal against the dark base.
- **Amber & System Red:** Used sparingly and strictly for warning and critical error states to ensure immediate user recognition.
- **Cool Grays:** Used for secondary text, inactive states, and structural dividers to create a clear hierarchy of information.

## Typography

This design system uses **Inter** for its exceptional legibility in technical interfaces and its neutral, modern tone. The typographic scale prioritizes "Data-First" readability.

For energy metrics (Voltage, Percentage, Time Remaining), use the `display-data` or `numeric-md` styles. These feature tighter letter spacing and heavier weights to ensure they are the first thing a user sees. Sub-labels and units should be set in `label-caps` to provide clear context without competing with the primary data values. All text must maintain a minimum contrast ratio of 4.5:1 against their respective backgrounds.

## Layout & Spacing

This design system employs an **8pt spacing system** to ensure mathematical harmony across all screen sizes. The layout model is a **fluid grid** for mobile interfaces and a **12-column fixed grid** for desktop/tablet dashboard views.

- **Margins:** 24px (3 units) for main container edges to provide breathing room.
- **Gutters:** 16px (2 units) between internal card elements.
- **Density:** High information density is permitted for data tables, but status dashboards should utilize generous vertical spacing (`stack-lg`) to differentiate between distinct power sources (Solar, Grid, Alternator).

## Elevation & Depth

Hierarchy is conveyed through **Tonal Layering** and **Low-Contrast Outlines**. In the dark technical interface, shadows are avoided in favor of varying surface brightness.

- **Level 0 (Background):** `#121212` – The base canvas.
- **Level 1 (Cards/Containers):** `#1E1E1E` – Used for primary data groupings.
- **Level 2 (Modals/Popovers):** `#2C2C2C` – The highest surface.

To define boundaries without adding visual noise, use 1px solid borders in a slightly lighter gray (`#333333`) or a very subtle 10% white opacity. This creates a "milled" or "machined" look consistent with industrial hardware.

## Shapes

The shape language balances industrial rigidity with modern software approachability. 

Standard components (Cards, Input Fields, Buttons) utilize a **10px radius** (`rounded-lg` in this system). This specific radius softens the technical interface while maintaining a disciplined, professional appearance. Circular shapes are reserved exclusively for status indicators, circular progress bars, and iconography containers to represent the "flow" of energy.

## Components

### Buttons
Primary buttons use the Technical Blue background with white text. Secondary buttons should be "Ghost" style with a 1px border. Always use `label-caps` for button text to ensure distinctiveness from body copy.

### Status Indicators (The "Power Rings")
Inspired by the reference image, use circular containers to represent power sources. 
- **Active:** 2px Blue or Green outer ring.
- **Inactive:** Solid Gray icon with 40% opacity.
- **Fault:** Pulsing 2px Red ring.

### Cards
Cards are the primary container for data. They should have a background of `surface` and a subtle 1px border. Metric cards should feature the large numeric value centered, with the label in `label-caps` positioned either directly above or below.

### Progress Bars & Gauges
Energy flow is visualized through linear and circular progress bars. Use a "Track and Fill" approach: the track is a dark gray (`#2C2C2C`) and the fill uses semantic colors (e.g., Green for battery charging, Blue for power output).

### Navigation
The sidebar/drawer uses a high-contrast dark background with simplified line icons. The Clayton Power logo must always be positioned in the top-left or top-center of the primary navigation container.