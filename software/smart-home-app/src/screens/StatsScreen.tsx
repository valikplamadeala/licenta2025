// src/screens/StatisticsScreen.tsx
import React, { useState, useEffect } from "react";
import { View, Text, StyleSheet, ScrollView } from "react-native";
import IntervalSelector from "../components/IntervalSelector";
import BarChartCard from "../components/BarChartCard";
import { getSensorStats, getCoolerUsage } from "../api/smartHomeApi"; // vezi mesajele anterioare
import { colors, spacing, font, radii } from '../utils/theme';

const INTERVALS = [
  { key: "day", label: "Zilnic" },
  { key: "week", label: "Săptămânal" },
  { key: "month", label: "Lunar" },
];

export default function StatisticsScreen() {
  type Stat = { label: string; avg: number; min: number; max: number };
  type Usage = { label: string; hours: number; cost: number };
  const [interval, setInterval] = useState("day");
  const [tempStats, setTempStats] = useState<Stat[]>([]);
  const [humStats, setHumStats] = useState<Stat[]>([]);
  const [gasStats, setGasStats] = useState<Stat[]>([]);
  const [coolerUsage, setCoolerUsage] = useState<Usage[]>([]);

  useEffect(() => {
    getSensorStats("temperature", interval).then(res => {
      setTempStats(res?.data?.stats || []);
      console.log('tempStats', res?.data?.stats);
    });
    getSensorStats("humidity", interval).then(res => {
      setHumStats(res?.data?.stats || []);
      console.log('humStats', res?.data?.stats);
    });
    getSensorStats("gas", interval).then(res => {
      setGasStats(res?.data?.stats || []);
      console.log('gasStats', res?.data?.stats);
    });
    getCoolerUsage(interval).then(res => {
      setCoolerUsage(res?.data?.usage || []);
      console.log('coolerUsage', res?.data?.usage);
    });
  }, [interval]);

  const isMock = tempStats.length === 7 && tempStats.every((d: any) => d.label && d.label.startsWith('Ziua'));

  // Helper pentru rotunjire la 2 zecimale
  const round2 = (v: number) => Math.round(v * 100) / 100;

  return (
    <ScrollView style={styles.root} contentContainerStyle={{ alignItems: "center" }}>
      {isMock && (
        <View >
        
        </View>
      )}
      <Text style={styles.title}>Statistici Smart Home</Text>
      <IntervalSelector value={interval} onChange={setInterval} options={INTERVALS} />
      {tempStats.length === 0 && <Text style={{color:'red'}}>Nu există date pentru temperatură!</Text>}
      <BarChartCard data={tempStats.map(s => ({ ...s, avg: round2(s.avg), min: round2(s.min), max: round2(s.max) }))} unit="°C" color="#3891f0" title="Temperatură (medie/zi)" />
      {humStats.length === 0 && <Text style={{color:'red'}}>Nu există date pentru umiditate!</Text>}
      <BarChartCard data={humStats.map(s => ({ ...s, avg: round2(s.avg), min: round2(s.min), max: round2(s.max) }))} unit="%" color="#67c1c2" title="Umiditate (medie/zi)" />
      {gasStats.length === 0 && <Text style={{color:'red'}}>Nu există date pentru gaz!</Text>}
      <BarChartCard data={gasStats.map(s => ({ ...s, avg: round2(s.avg), min: round2(s.min), max: round2(s.max) }))} unit="ppm" color="#f0b239" title="Gaz (medie/zi)" />
      {coolerUsage.length === 0 && <Text style={{color:'red'}}>Nu există date pentru cooler!</Text>}
      <BarChartCard data={coolerUsage.map(u => ({ label: u.label, value: round2(u.hours) }))} unit="h" color="#67c1c2" title="Cooler ON (ore/zi)" />
      <BarChartCard data={coolerUsage.map(u => ({ label: u.label, value: round2(u.cost) }))} unit=" lei" color="#f0b239" title="Cost Estimat Cooler (lei/zi)" />
    </ScrollView>
  );
}
const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: colors.background },
  title: { fontSize: font.size.xl, fontWeight: "bold", marginTop: spacing.lg, color: colors.primary, letterSpacing: 0.2 },
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