import React, { useEffect, useRef } from 'react';
import { Animated, Easing, Pressable, StyleSheet, Text, View } from 'react-native';
import Svg, { Path } from 'react-native-svg';
import CarbonIcon from './CarbonIcon';
import { colors, type } from '../utils/theme';

// 270° gauge (spec §6.2) — geometry identical to the display mockup:
// 64-unit viewBox, radius 24, stroke 6, starting at 225° and sweeping clockwise.
function pt(angle) {
  const r = (angle * Math.PI) / 180;
  return [32 + 24 * Math.cos(r), 32 - 24 * Math.sin(r)];
}

function arcPath(a0, a1) {
  const [x0, y0] = pt(a0);
  const [x1, y1] = pt(a1);
  const large = a0 - a1 > 180 ? 1 : 0;
  return `M ${x0.toFixed(2)} ${y0.toFixed(2)} A 24 24 0 ${large} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
}

const TRACK = arcPath(225, -45);

const ICON_COLOR = {
  on: colors.blue,
  off: colors.dim,
  blocked: colors.red,
  warn: colors.yellow,
  overload: colors.yellow,
};

/**
 * Firmware set_dial()/func_color() port: derive state + fill from the CAN
 * function state, failure byte, live power and the gauge capacity.
 */
export function dialState(state, fail, powerW, capW) {
  if (state >= 1 && fail < 3 && capW > 0 && powerW > capW) return { state: 'overload', pct: 1 };
  if (fail >= 3) return { state: 'blocked', pct: 0 };
  if (state < 1) return { state: 'off', pct: 0 };
  const pct = capW > 0 ? Math.max(0, Math.min(1, powerW / capW)) : 0;
  return { state: fail === 2 ? 'warn' : 'on', pct };
}

function usePulse(active) {
  const opacity = useRef(new Animated.Value(1)).current;
  useEffect(() => {
    if (!active) {
      opacity.setValue(1);
      return undefined;
    }
    const loop = Animated.loop(
      Animated.sequence([
        Animated.timing(opacity, { toValue: 0.4, duration: 500, easing: Easing.inOut(Easing.ease), useNativeDriver: true }),
        Animated.timing(opacity, { toValue: 1, duration: 500, easing: Easing.inOut(Easing.ease), useNativeDriver: true }),
      ]),
    );
    loop.start();
    return () => loop.stop();
  }, [active, opacity]);
  return opacity;
}

// Round plate around controllable gauges: 7 dp padding + 1 dp border.
const PLATE_INSET = 8;

/**
 * `size` is the outer diameter. With onPress the gauge sits on a round plate
 * inside that diameter, so plated and plain gauges line up as equal circles.
 */
export default function Dial({ size: outer = 100, icon, state = 'off', pct = 0, powerW = 0, onPress }) {
  const size = onPress ? outer - 2 * PLATE_INSET : outer;
  const overload = state === 'overload';
  const zero = state === 'off' || state === 'blocked';
  const fill = overload ? 1 : zero ? 0 : pct;
  const pulse = usePulse(overload);
  const iconSize = Math.round(size * 0.34);

  const gauge = (
    <View style={{ width: size, height: size }}>
      <Svg width={size} height={size} viewBox="0 0 64 64" style={StyleSheet.absoluteFill}>
        <Path d={TRACK} stroke={colors.track} strokeWidth={6} fill="none" />
        {!overload && fill > 0.004 && (
          <Path d={arcPath(225, 225 - 270 * fill)} stroke={colors.ink} strokeWidth={6} fill="none" />
        )}
      </Svg>
      {overload && (
        <Animated.View style={[StyleSheet.absoluteFill, { opacity: pulse }]}>
          <Svg width={size} height={size} viewBox="0 0 64 64">
            <Path d={TRACK} stroke={colors.yellow} strokeWidth={6} fill="none" />
          </Svg>
        </Animated.View>
      )}
      <View style={[StyleSheet.absoluteFill, styles.center]}>
        <CarbonIcon name={icon} size={iconSize} color={ICON_COLOR[state] || colors.dim} />
      </View>
    </View>
  );

  return (
    <View style={styles.cell}>
      {onPress ? (
        <Pressable
          onPress={onPress}
          style={({ pressed }) => [styles.plate, pressed && styles.platePressed]}
          hitSlop={6}
        >
          {gauge}
        </Pressable>
      ) : gauge}
      <Text style={[type.value, zero && styles.dimValue]}>
        {zero ? 0 : Math.round(powerW)}
        <Text style={styles.unit}> W</Text>
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  cell: { alignItems: 'center', gap: 4 },
  center: { alignItems: 'center', justifyContent: 'center' },
  plate: {
    padding: 7,
    borderRadius: 999,
    backgroundColor: colors.panel,
    borderWidth: 1,
    borderColor: colors.edge,
  },
  platePressed: { backgroundColor: colors.panelPressed },
  dimValue: { color: colors.dim },
  unit: { fontSize: 11, color: colors.dim },
});
