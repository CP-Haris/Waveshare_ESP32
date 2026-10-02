import React, { useEffect, useRef, useState } from 'react';
import { Modal, ScrollView, StyleSheet, Text, View } from 'react-native';
import CarbonIcon from './CarbonIcon';
import { Button, IconButton, Sheet } from './Carbon';
import { colors, font, spacing, type } from '../utils/theme';
import { ERROR_LEVEL, activeErrorDefinitions } from '../utils/errorCodes';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';

// Error list sheet + auto popup (spec §7). Mounted once at the app root;
// the status bar badge opens the list through openErrorList().

const listeners = new Set();
let openRequested = false;

export function openErrorList() {
  openRequested = true;
  listeners.forEach((l) => l());
}

export function levelColor(level) {
  if (level === ERROR_LEVEL.WARNING) return colors.yellow;
  if (level === ERROR_LEVEL.FAILURE || level === ERROR_LEVEL.CRITICAL) return colors.red;
  return colors.dim;
}

/** Worst level among the definitions: red beats yellow beats dim. */
export function worstColor(definitions) {
  if (definitions.some((d) => d.level === ERROR_LEVEL.FAILURE || d.level === ERROR_LEVEL.CRITICAL)) return colors.red;
  if (definitions.some((d) => d.level === ERROR_LEVEL.WARNING)) return colors.yellow;
  return definitions.length ? colors.dim : colors.faint;
}

const isPopupLevel = (d) => d.level === ERROR_LEVEL.FAILURE || d.level === ERROR_LEVEL.CRITICAL;

function splitTitle(title) {
  // "E020 230 VAC Overload" → ["E020", "230 VAC OVERLOAD"]
  const m = String(title).match(/^(E\d+)\s+(.*)$/);
  return m ? [m[1], m[2].toUpperCase()] : ['', String(title).toUpperCase()];
}

export default function ErrorCenter() {
  const [definitions, setDefinitions] = useState([]);
  const [listVisible, setListVisible] = useState(false);
  const [popupQueue, setPopupQueue] = useState([]);
  const [clearing, setClearing] = useState(false);
  const [clearStatus, setClearStatus] = useState('');
  const activeCodesRef = useRef(new Set());

  useEffect(() => {
    const onOpen = () => {
      if (!openRequested) return;
      openRequested = false;
      setClearStatus('');
      setListVisible(true);
    };
    listeners.add(onOpen);

    const unsub = canGatewayService.onNotification((msg) => {
      if (msg.type !== 'dashboard' && msg.type !== 'errors') return;
      const codes = msg.type === 'dashboard' ? msg.data.errorCodes : msg.data;
      const defs = activeErrorDefinitions(codes);
      setDefinitions(defs);

      const previous = activeCodesRef.current;
      const fresh = defs.filter((d) => isPopupLevel(d) && !previous.has(d.code));
      activeCodesRef.current = new Set(defs.map((d) => d.code));
      if (fresh.length) setPopupQueue((q) => [...q, ...fresh.filter((d) => !q.some((p) => p.code === d.code))]);
    });

    const unsubConn = bleService.onConnectionChange((connected) => {
      if (connected) return;
      activeCodesRef.current = new Set();
      setDefinitions([]);
      setPopupQueue([]);
      setListVisible(false);
    });

    return () => {
      listeners.delete(onOpen);
      unsub();
      unsubConn();
    };
  }, []);

  const handleClear = async () => {
    if (clearing || definitions.length === 0) return;
    setClearing(true);
    setClearStatus('');
    try {
      const ok = await canGatewayService.clearErrors();
      setClearStatus(ok ? 'Clear command sent' : 'Unable to send clear command');
      if (ok) {
        canGatewayService.requestErrors();
        canGatewayService.requestDashboard();
      }
    } catch (e) {
      setClearStatus('Unable to send clear command');
    } finally {
      setClearing(false);
    }
  };

  const popup = !listVisible && popupQueue[0];
  const [popupCode, popupTitle] = popup ? splitTitle(popup.title) : ['', ''];

  return (
    <>
      <Sheet
        visible={listVisible}
        onClose={() => setListVisible(false)}
        title={definitions.length ? `ERRORS · ${definitions.length}` : 'ERRORS'}
        right={<IconButton icon="close" onPress={() => setListVisible(false)} />}
      >
        <ScrollView style={styles.list}>
          {definitions.length === 0 ? (
            <View style={styles.empty}>
              <CarbonIcon name="check" size={32} color={colors.dim} />
              <Text style={[type.label, { color: colors.dim }]}>No active errors</Text>
            </View>
          ) : definitions.map((d) => {
            const [code, title] = splitTitle(d.title);
            return (
              <View key={d.code} style={[styles.item, { borderLeftColor: levelColor(d.level) }]}>
                <Text style={styles.itemTitle}>
                  <Text style={{ color: levelColor(d.level) }}>{code}</Text>  ·  {title}
                </Text>
                <Text style={type.body}>{d.description}</Text>
              </View>
            );
          })}
        </ScrollView>
        {!!clearStatus && <Text style={[type.small, styles.clearStatus]}>{clearStatus}</Text>}
        <Button
          label="CLEAR ERRORS"
          variant="danger"
          onPress={handleClear}
          loading={clearing}
          disabled={definitions.length === 0}
          style={styles.clearBtn}
        />
      </Sheet>

      <Modal visible={!!popup} transparent animationType="fade" statusBarTranslucent onRequestClose={() => setPopupQueue((q) => q.slice(1))}>
        <View style={styles.popScrim}>
          <View style={styles.pop}>
            <View style={styles.popHead}>
              <CarbonIcon name="warn" size={22} color={colors.red} />
              <Text style={styles.popTitle}>{popupCode} · {popupTitle}</Text>
            </View>
            <Text style={[type.body, styles.popBody]}>{popup?.description}</Text>
            <View style={styles.popActions}>
              <Button label="OK" compact onPress={() => setPopupQueue((q) => q.slice(1))} style={styles.popOk} />
            </View>
          </View>
        </View>
      </Modal>
    </>
  );
}

const styles = StyleSheet.create({
  list: { maxHeight: 420, marginTop: spacing.sm },
  empty: { alignItems: 'center', gap: spacing.sm, paddingVertical: spacing.xl },
  item: {
    borderLeftWidth: 4,
    backgroundColor: colors.bg,
    paddingVertical: spacing.sm + 4,
    paddingHorizontal: spacing.md,
    marginBottom: spacing.sm,
    gap: 4,
  },
  itemTitle: { fontFamily: font.bold, fontSize: 16, letterSpacing: 1, color: colors.ink },
  clearStatus: { textAlign: 'center', marginTop: spacing.sm },
  clearBtn: { marginTop: spacing.md },

  popScrim: { flex: 1, backgroundColor: colors.scrim, alignItems: 'center', justifyContent: 'center', padding: spacing.md },
  pop: {
    width: '100%',
    maxWidth: 430,
    backgroundColor: colors.panel,
    borderWidth: 1,
    borderColor: colors.edge,
    borderLeftWidth: 4,
    borderLeftColor: colors.red,
    paddingHorizontal: spacing.lg,
    paddingTop: spacing.lg - 4,
    paddingBottom: spacing.md + 2,
  },
  popHead: { flexDirection: 'row', alignItems: 'center', gap: 10 },
  popTitle: { flex: 1, fontFamily: font.bold, fontSize: 16, letterSpacing: 1.3, color: colors.red },
  popBody: { marginTop: 10, color: '#C9CBC6' },
  popActions: { marginTop: 18, flexDirection: 'row', justifyContent: 'flex-end' },
  popOk: { minWidth: 96 },
});
