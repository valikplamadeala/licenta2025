import React, { useEffect, useState } from "react";
import { View, ScrollView, Text, StyleSheet, Alert, RefreshControl } from "react-native";
import DeviceCard from "../components/DeviceCard";
import { getDeviceState, setDeviceState } from "../api/smartHomeApi";
import { colors, spacing, font } from '../utils/theme';

import { MaterialCommunityIcons } from '@expo/vector-icons';

const DEVICES: Array<{
  key: string;
  name: string;
  icon: keyof typeof MaterialCommunityIcons.glyphMap;
}> = [
  { key: "kitchen", name: "Bec Bucătărie", icon: "lightbulb" },
  { key: "living", name: "Bec Living", icon: "lightbulb-group" },
  { key: "bedroom", name: "Bec Dormitor", icon: "lightbulb-on-outline" },
  { key: "cooler", name: "Cooler", icon: "fan" },
];

export default function DevicesScreen() {
  const [states, setStates] = useState<{ [key: string]: boolean }>({});
  const [loading, setLoading] = useState(false);

  const fetchStates = async () => {
    setLoading(true);
    let s: any = {};
    for (const device of DEVICES) {
      try {
        const res = await getDeviceState(device.key);
        s[device.key] = res.data[device.key] === "on";
      } catch {
        s[device.key] = false;
      }
    }
    setStates(s);
    setLoading(false);
  };

  useEffect(() => {
    fetchStates();
  }, []);

  const handleToggle = async (deviceKey: string, value: boolean) => {
    setStates((prev) => ({ ...prev, [deviceKey]: value }));
    try {
      await setDeviceState(deviceKey, value ? "on" : "off");
    } catch {
      Alert.alert("Eroare", "Nu s-a putut trimite comanda.");
    }
  };

  return (
    <ScrollView
      contentContainerStyle={styles.container}
      refreshControl={<RefreshControl refreshing={loading} onRefresh={fetchStates} />}
    >
      <Text style={styles.title}>Control Device-uri</Text>
      {DEVICES.map((dev) => (
        <DeviceCard
          key={dev.key}
          name={dev.name}
          icon={dev.icon}
          isOn={!!states[dev.key]}
          onToggle={(val) => handleToggle(dev.key, val)}
        />
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: "center", paddingVertical: spacing.lg, backgroundColor: colors.background },
  title: { fontSize: font.size.xl, fontWeight: "bold", marginVertical: spacing.md, color: colors.primary, letterSpacing: 0.2 },
});