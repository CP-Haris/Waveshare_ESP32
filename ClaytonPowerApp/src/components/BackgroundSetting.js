import React, { useEffect, useState } from 'react';
import { Linking, Platform, StyleSheet, Text, View } from 'react-native';
import { Button, Notice, Row, Section, Toggle } from './Carbon';
import { isAvailable as foregroundServiceAvailable } from '../../modules/ble-foreground';
import { backgroundService } from '../services/backgroundService';
import { colors, spacing, type } from '../utils/theme';

/** Settings › APP: the "background notifications" switch. */
export default function BackgroundSetting() {
  const [state, setState] = useState(backgroundService.state);
  const [refused, setRefused] = useState(false);

  useEffect(() => backgroundService.subscribe(setState), []);

  const toggle = async (value) => {
    setRefused(false);
    try {
      const ok = await backgroundService.setEnabled(value);
      if (!ok) setRefused(true);
    } catch (error) {
      console.warn('[Background] could not change setting:', error.message);
    }
  };

  const unitName = state.lastDevice?.name;
  const tag = state.enabled
    ? unitName ? `MONITORING ${unitName.toUpperCase()}` : 'ON · CONNECT TO A UNIT TO START'
    : 'OFF';

  return (
    <Section title="APP">
      <Row
        kind="item"
        icon="bt"
        iconColor={state.enabled ? colors.blue : colors.dim}
        label="Background notifications"
        tag={tag}
        tagColor={state.enabled ? colors.blue : colors.dim}
        right={(
          <Toggle
            value={state.enabled}
            onValueChange={toggle}
            disabled={Platform.OS === 'android' && !foregroundServiceAvailable}
          />
        )}
        last
      />
      <Text style={[type.small, styles.help]}>
        Stays connected to the last unit when the app is closed, reconnects when you are nearby, and
        notifies you about errors, low battery and a full charge. Android shows a permanent
        notification while this is on.
      </Text>
      {refused && (
        <View>
          <Notice text="Notifications are blocked for this app. Allow them in the phone's settings." />
          <Button compact label="OPEN SETTINGS" onPress={() => Linking.openSettings()} style={styles.btn} />
        </View>
      )}
    </Section>
  );
}

const styles = StyleSheet.create({
  help: { lineHeight: 18, marginTop: spacing.xs, marginBottom: spacing.sm },
  btn: { alignSelf: 'flex-start', marginBottom: spacing.sm },
});
