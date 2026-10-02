import React, { useEffect, useState } from 'react';
import { StyleSheet, Text, View, useWindowDimensions } from 'react-native';
import StatusBar from '../components/StatusBar';
import CarbonIcon from '../components/CarbonIcon';
import Dial, { dialState } from '../components/Dial';
import Chevrons from '../components/Chevrons';
import ForecastChart from '../components/ForecastChart';
import { Button } from '../components/Carbon';
import { colors, font, spacing, type } from '../utils/theme';
import { unitFamily } from '../utils/units';
import { recordSoc } from '../services/socHistory';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';

// Gauge full scale until CAN-provided maxima are wired in — same fallbacks
// as the firmware's CAP_* defines (spec §6.2).
const CAP = { acIn: 2000, dcIn: 1000, solar: 800, acOut: 3000, dcOut: 1200 };

const MARGIN = spacing.md;
const MIN_CHART_H = 110;

function finite(value) {
  return Number.isFinite(value) ? value : 0;
}

function hhmm(date) {
  return `${String(date.getHours()).padStart(2, '0')}:${String(date.getMinutes()).padStart(2, '0')}`;
}

/** Forecast (spec §6), stacked beside the SoC: "FULL AT 18:01" / "IN 3H 13M". */
function ForecastLine({ charging, discharging, minutesLeft }) {
  if ((charging || discharging) && minutesLeft > 0) {
    const at = hhmm(new Date(Date.now() + minutesLeft * 60000));
    const h = Math.floor(minutesLeft / 60);
    const m = String(minutesLeft % 60).padStart(2, '0');
    return (
      <View style={styles.forecast}>
        <Text style={styles.untilKey}>{charging ? 'FULL' : 'EMPTY'} AT {at}</Text>
        <Text style={styles.until}>IN {h}H {m}M</Text>
      </View>
    );
  }
  return (
    <View style={styles.forecast}>
      <Text style={charging ? styles.untilKey : styles.until}>
        {charging ? 'CHARGING' : discharging ? 'DISCHARGING' : 'STANDBY'}
      </Text>
    </View>
  );
}

function ZoneHead({ title, total, dirIn, active }) {
  return (
    <View style={styles.zoneHead}>
      <View style={styles.zoneTitle}>
        <Text style={type.zone}>{title}</Text>
        <Chevrons dirIn={dirIn} visible={active} />
      </View>
      <View style={styles.total}>
        <Text style={[type.total, !active && styles.dim]}>{Math.round(total)}</Text>
        <Text style={styles.totalUnit}>W</Text>
      </View>
    </View>
  );
}

export default function DashboardScreen({ navigation }) {
  const { width } = useWindowDimensions();
  const [bodyHeight, setBodyHeight] = useState(0);
  const [data, setData] = useState(null);
  const [history, setHistory] = useState([]);
  const [connected, setConnected] = useState(bleService.isConnected);

  useEffect(() => {
    const unsubData = canGatewayService.onNotification((msg) => {
      if (msg.type !== 'dashboard') return;
      const soc = Math.max(0, Math.min(100, Math.round(finite(msg.data.soc))));
      const unitKey = msg.data.serial || msg.data.partNumber || 'unit';
      setHistory(recordSoc(unitKey, soc));
      setData(msg.data);
    });
    const unsubConn = bleService.onConnectionChange((c) => {
      setConnected(c);
      if (!c) setData(null);
    });
    if (bleService.isConnected) canGatewayService.requestDashboard();
    return () => {
      unsubData();
      unsubConn();
    };
  }, []);

  if (!connected || !data) {
    return (
      <View style={styles.screen}>
        <StatusBar />
        <View style={styles.center}>
          <CarbonIcon name="bt" size={48} color={connected ? colors.blue : colors.faint} />
          <Text style={[type.zone, styles.centerTitle]}>{connected ? 'WAITING FOR DATA' : 'NOT CONNECTED'}</Text>
          {!connected && (
            <Button label="CONNECT" variant="primary" onPress={() => navigation.navigate('Connect')} style={styles.centerBtn} />
          )}
        </View>
      </View>
    );
  }

  const d = data;
  const isBms = unitFamily({ type: d.unitType, partNumber: d.partNumber }) === 'bms';
  const soc = Math.max(0, Math.min(100, Math.round(finite(d.soc))));
  const charging = finite(d.batteryCurrent) > 0.5;
  const discharging = finite(d.batteryCurrent) < -0.5;
  const minutesLeft = Math.abs(Math.round(finite(d.socTimeMin)));

  const pAcIn = finite(d.acInPower);
  const pDcIn = Math.max(0, finite(d.dcInVoltage) * finite(d.dcInCurrent));
  const pSolar = Math.max(0, finite(d.solarCurrent) * finite(d.batteryVoltage));
  const pAcOut = finite(d.acOutPower);
  const pDcOut = Math.max(0, finite(d.dcOutVoltage) * finite(d.dcOutCurrent));

  const acIn = dialState(d.chargerState, d.chargerFail, pAcIn, CAP.acIn);
  const dcIn = dialState(d.dcInState, d.dcInFail, pDcIn, CAP.dcIn);
  const solar = dialState(d.solarState, d.solarFail, pSolar, CAP.solar);
  const acOut = dialState(d.inverterState, d.inverterFail, pAcOut, CAP.acOut);
  const dcOut = dialState(d.dcOutState, d.dcOutFail, pDcOut, CAP.dcOut);

  const shown = (s, p) => (s.state === 'off' || s.state === 'blocked' ? 0 : p);
  const chgTotal = shown(acIn, pAcIn) + shown(dcIn, pDcIn) + shown(solar, pSolar);
  const disTotal = (isBms ? 0 : shown(acOut, pAcOut)) + shown(dcOut, pDcOut);

  // The dashboard never scrolls. Measure the real space between status bar
  // and tab bar, give the gauges what is left after the fixed rows plus a
  // minimum chart height, and let the chart absorb the remainder.
  const compact = bodyHeight > 0 && bodyHeight < 640;
  const socH = compact ? 72 : 84;
  const fixedH = 3 * 24 + 18 + socH + 8 // battery zone: padding, label, SoC
    + 2 * (44 + 8 + 30)                 // two zone heads + gauge value lines
    + MIN_CHART_H + 8 + 10;             // chart, its top margin, dividers/rounding
  const zones = isBms ? 1 : 2;
  const byHeight = bodyHeight > 0 ? Math.floor((bodyHeight - fixedH) / zones) : 90;
  const byWidth = Math.floor((width - 2 * MARGIN - 2 * 16) / 3);
  const dialSize = Math.max(56, Math.min(90, byWidth, byHeight));

  return (
    <View style={styles.screen}>
      <StatusBar />
      <View style={styles.body} onLayout={(e) => setBodyHeight(e.nativeEvent.layout.height)}>
        <View style={[styles.zone, styles.batteryZone]}>
          <Text style={type.zone}>BATTERY</Text>
          <View style={styles.socRow}>
            <View style={styles.soc}>
              <Text style={[type.soc, compact && styles.socCompact]}>{soc}</Text>
              <Text style={[styles.socUnit, compact && styles.socUnitCompact]}>%</Text>
            </View>
            <ForecastLine charging={charging} discharging={discharging} minutesLeft={minutesLeft} />
          </View>
          <ForecastChart
            history={history}
            soc={soc}
            charging={charging}
            discharging={discharging}
            minutesLeft={minutesLeft}
            style={styles.chart}
          />
        </View>

        {!isBms && (
          <View style={[styles.zone, styles.zoneDivider]}>
            <ZoneHead title="CHARGING" total={chgTotal} dirIn active={chgTotal > 5} />
            <View style={styles.dialRow}>
              <Dial size={dialSize} icon="plug" {...acIn} powerW={pAcIn} />
              <Dial size={dialSize} icon="car" {...dcIn} powerW={pDcIn} />
              <Dial size={dialSize} icon="sun" {...solar} powerW={pSolar} />
            </View>
          </View>
        )}

        <View style={[styles.zone, styles.zoneDivider]}>
          <ZoneHead title="DISCHARGING" total={disTotal} dirIn={false} active={disTotal > 5} />
          {/* Same three-column grid as CHARGING: outputs sit under the outer
              columns (plug / sun); a BMS shows DC out alone in the middle. */}
          <View style={styles.dialRow}>
            {isBms ? <View style={{ width: dialSize }} /> : (
              <Dial size={dialSize} icon="socket" {...acOut} powerW={pAcOut} onPress={() => canGatewayService.toggleFunc(0)} />
            )}
            {isBms ? (
              <Dial size={dialSize} icon="dc" {...dcOut} powerW={pDcOut} onPress={() => canGatewayService.toggleFunc(1)} />
            ) : <View style={{ width: dialSize }} />}
            {isBms ? <View style={{ width: dialSize }} /> : (
              <Dial size={dialSize} icon="dc" {...dcOut} powerW={pDcOut} onPress={() => canGatewayService.toggleFunc(1)} />
            )}
          </View>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: colors.bg },
  body: { flex: 1 },
  center: { flex: 1, alignItems: 'center', justifyContent: 'center', gap: spacing.md, padding: spacing.xl },
  centerTitle: { color: colors.dim },
  centerBtn: { marginTop: spacing.sm, minWidth: 200 },

  zone: { paddingHorizontal: MARGIN, paddingTop: 12, paddingBottom: 12 },
  batteryZone: { flex: 1 },
  chart: { flex: 1, marginTop: spacing.sm },
  zoneDivider: { borderTopWidth: 1, borderTopColor: colors.line },

  socRow: { flexDirection: 'row', alignItems: 'flex-end', justifyContent: 'space-between' },
  soc: { flexDirection: 'row', alignItems: 'flex-end' },
  socCompact: { fontSize: 72, lineHeight: 72 },
  socUnit: { fontFamily: font.semibold, fontSize: 42, lineHeight: 42, color: colors.dim, marginLeft: 3, marginBottom: 8 },
  socUnitCompact: { fontSize: 36, lineHeight: 36, marginBottom: 6 },
  forecast: { alignItems: 'flex-end', paddingBottom: 12, gap: 2 },
  until: { fontFamily: font.semibold, fontSize: 14, letterSpacing: 0.6, color: colors.soft },
  untilKey: { fontFamily: font.bold, fontSize: 16, letterSpacing: 0.6, color: colors.blue },

  zoneHead: { flexDirection: 'row', alignItems: 'flex-start', justifyContent: 'space-between' },
  zoneTitle: { flexDirection: 'row', alignItems: 'center', gap: 12, paddingTop: 4 },
  total: { flexDirection: 'row', alignItems: 'flex-start', gap: 4 },
  totalUnit: { fontFamily: font.semibold, fontSize: 14, color: colors.dim, marginTop: 5 },
  dim: { color: colors.dim },

  dialRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: spacing.sm },
});
