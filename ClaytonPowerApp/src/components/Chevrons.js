import React, { useEffect, useRef } from 'react';
import { Animated, Easing, StyleSheet, View } from 'react-native';
import Svg, { Path } from 'react-native-svg';
import { colors } from '../utils/theme';

// Marching chevrons (spec §6.3): three strokes, opacity 0.3 → 1 → 0.3 over
// 1.8 s, each offset by 0.25 s. dirIn points toward the battery (left).
function Chevron({ dirIn, delay }) {
  const opacity = useRef(new Animated.Value(0.3)).current;

  useEffect(() => {
    const loop = Animated.loop(
      Animated.sequence([
        Animated.delay(delay),
        Animated.timing(opacity, { toValue: 1, duration: 720, easing: Easing.inOut(Easing.ease), useNativeDriver: true }),
        Animated.timing(opacity, { toValue: 0.3, duration: 1080, easing: Easing.inOut(Easing.ease), useNativeDriver: true }),
        Animated.delay(500 - delay),
      ]),
    );
    loop.start();
    return () => loop.stop();
  }, [delay, opacity]);

  return (
    <Animated.View style={{ opacity }}>
      <Svg width={8} height={12} viewBox="0 0 8 12">
        <Path
          d={dirIn ? 'M7 1 L1 6 L7 11' : 'M1 1 L7 6 L1 11'}
          stroke={colors.ink}
          strokeWidth={2.2}
          strokeLinecap="round"
          strokeLinejoin="round"
          fill="none"
        />
      </Svg>
    </Animated.View>
  );
}

export default function Chevrons({ dirIn, visible = true }) {
  if (!visible) return null;
  // The light sweeps in the direction of flow: right-to-left when charging
  // (toward the battery), left-to-right when discharging.
  const delays = dirIn ? [500, 250, 0] : [0, 250, 500];
  return (
    <View style={styles.row}>
      {delays.map((d, i) => <Chevron key={i} dirIn={dirIn} delay={d} />)}
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 1, alignItems: 'center' },
});
