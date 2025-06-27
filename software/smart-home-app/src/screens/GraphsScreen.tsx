import React, { useState, useEffect } from "react";
import { View, Text, StyleSheet, TouchableOpacity, ActivityIndicator, ScrollView } from "react-native";
import SensorChart from "../components/SensorChart";
import { getTemperatureHistory, getHumidityHistory, getGasHistory } from "../api/smartHomeApi";
import { colors, spacing, font, radii } from '../utils/theme';

const TABS = [
  { key: "temperature", label: "Temperatură", unit: "°C", color: colors.primary },
  { key: "humidity", label: "Umiditate", unit: "%", color: colors.secondary },
  { key: "gaz", label: "Gaz", unit: "ppm", color: colors.accent },
];

export default function GraphsScreen() {
  const [tab, setTab] = useState(TABS[0].key);
  const [data, setData] = useState<any[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let fetcher = getTemperatureHistory;
    if (tab === "humidity") fetcher = getHumidityHistory;
    if (tab === "gaz") fetcher = getGasHistory;
    setLoading(true);
    fetcher().then((res) => {
      setData(res.data.history || []);
      setLoading(false);
    });
  }, [tab]);

  const currentTab = TABS.find((t) => t.key === tab);

  const isMock = data.length === 10 && data.every(d => d.timestamp && d.timestamp.includes('T'));

  return (
    <View style={styles.root}>
      {isMock && (
        <View style={styles.demoBadge}>
          
        </View>
      )}
      <Text style={styles.title}>Grafic {currentTab?.label}</Text>
      <View style={styles.tabs}>
        {TABS.map((t) => (
          <TouchableOpacity
            key={t.key}
            style={[
              styles.tabBtn,
              tab === t.key && { backgroundColor: t.color + "18", borderColor: t.color }
            ]}
            onPress={() => setTab(t.key)}
          >
            <Text style={[
              styles.tabText,
              tab === t.key && { color: t.color, fontWeight: "bold" }
            ]}>
              {t.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>
      <ScrollView contentContainerStyle={{ alignItems: "center" }}>
        {loading ? (
          <ActivityIndicator size="large" color={colors.primary} style={{ marginTop: 60 }} />
        ) : (
          <SensorChart
            title={currentTab?.label || ""}
            data={data}
            color={currentTab?.color}
            unit={currentTab?.unit}
          />
        )}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: colors.background },
  title: { fontSize: font.size.xl, fontWeight: "bold", marginTop: spacing.lg, marginBottom: spacing.sm, textAlign: "center", color: colors.primary, letterSpacing: 0.2 },
  tabs: { flexDirection: "row", justifyContent: "center", marginBottom: spacing.sm, marginTop: 4 },
  tabBtn: {
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.sm,
    borderRadius: radii.lg,
    marginHorizontal: spacing.xs,
    borderWidth: 1.5,
    borderColor: colors.border,
    backgroundColor: colors.card,
  },
  tabText: { fontSize: font.size.sm, color: colors.textSecondary, fontWeight: '600' },
  demoBadge: {
    position: 'absolute',
    top: spacing.sm,
    right: spacing.sm,
    backgroundColor: colors.accent,
    borderRadius: radii.sm,
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.xs,
    zIndex: 10,
    shadowColor: colors.accent,
    shadowOpacity: 0.18,
    shadowRadius: 8,
    elevation: 6,
  },
  demoBadgeText: {
    color: colors.card,
    fontWeight: 'bold',
    fontSize: font.size.sm,
    letterSpacing: 1.2,
  },
});