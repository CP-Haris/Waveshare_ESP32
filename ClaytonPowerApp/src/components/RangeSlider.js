import React, { useMemo, useRef, useState } from 'react';
import { PanResponder, StyleSheet, View } from 'react-native';
import { colors } from '../utils/theme';

const TRACK_H = 6;
const THUMB = 24;
const TOUCH_H = 44;

function snap(value, min, max, step) {
  const stepped = step > 0 ? min + Math.round((value - min) / step) * step : value;
  return Math.max(min, Math.min(max, stepped));
}

/**
 * The min–max bar in the setting editor, draggable (spec §8). Tap anywhere
 * on it or drag the thumb; values snap to `step` (same as the −/+ buttons).
 * Without a known range it is shown as a plain, inactive track.
 */
export default function RangeSlider({ min, max, value, step, onChange }) {
  const [width, setWidth] = useState(0);
  const startX = useRef(0);
  const ready = width > 0 && min != null && max != null && max > min;

  // The responder is created once, so read the latest props through a ref.
  const latest = useRef({});
  latest.current = { min, max, step, width, onChange, ready };

  const responder = useMemo(() => {
    const valueAt = (x) => {
      const { min: lo, max: hi, step: st, width: w } = latest.current;
      const fraction = Math.max(0, Math.min(1, x / w));
      return snap(lo + fraction * (hi - lo), lo, hi, st);
    };
    return PanResponder.create({
      onStartShouldSetPanResponder: () => latest.current.ready,
      onMoveShouldSetPanResponder: () => latest.current.ready,
      onPanResponderTerminationRequest: () => false,
      onPanResponderGrant: (evt) => {
        startX.current = evt.nativeEvent.locationX;
        latest.current.onChange(valueAt(startX.current));
      },
      onPanResponderMove: (evt, gesture) => {
        latest.current.onChange(valueAt(startX.current + gesture.dx));
      },
    });
  }, []);

  const fraction = ready ? Math.max(0, Math.min(1, (value - min) / (max - min))) : 0;

  return (
    <View
      style={styles.touch}
      onLayout={(e) => setWidth(e.nativeEvent.layout.width)}
      {...responder.panHandlers}
      accessibilityRole="adjustable"
    >
      <View style={styles.track} pointerEvents="none">
        <View style={[styles.fill, { width: `${fraction * 100}%` }]} />
      </View>
      {ready && (
        <View
          pointerEvents="none"
          style={[styles.thumb, { left: fraction * width - THUMB / 2 }]}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  touch: { height: TOUCH_H, justifyContent: 'center' },
  track: { height: TRACK_H, backgroundColor: colors.track },
  fill: { height: TRACK_H, backgroundColor: colors.blue },
  thumb: {
    position: 'absolute',
    top: (TOUCH_H - THUMB) / 2,
    width: THUMB,
    height: THUMB,
    borderRadius: THUMB / 2,
    backgroundColor: colors.ink,
    borderWidth: 4,
    borderColor: colors.blue,
  },
});
