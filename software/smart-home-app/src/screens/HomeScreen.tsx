import React from "react";
import { View, Text, StyleSheet, ScrollView } from "react-native";
import useLiveStatus from "../hooks/useLiveStatus";
import { Ionicons, Feather, MaterialCommunityIcons } from "@expo/vector-icons";
import { colors, spacing, font, radii } from '../utils/theme';

const GAZ_LIMIT = 400; // ppm - ajustează dacă ai alt prag

export default function HomeScreen() {
  const { data, isMock } = useLiveStatus();

  if (!data || !data.sensors || !data.devices) {
    return (
      <View style={styles.container}>
        <Text style={styles.text}>Se încarcă datele...</Text>
        {data && (!data.sensors || !data.devices) && (
          <Text style={[styles.text, { color: 'red', marginTop: 10 }]}>Eroare: structura datelor este invalidă!</Text>
        )}
      </View>
    );
  }

  const gazVal = Number(data.sensors.esp32_gaz);

  return (
    <ScrollView contentContainerStyle={styles.container}>
      {/* Badge DEMO dacă datele sunt mock */}
      {isMock && (
        <View style={styles.demoBadge}>
         
        </View>
      )}
      {/* Alertă gaz */}
      {gazVal > GAZ_LIMIT && (
        <View style={styles.alertBox}>
          <MaterialCommunityIcons name="alert-octagon" size={32} color={colors.card} />
          <Text style={styles.alertText}>
            PERICOL: Gaz peste limită ({gazVal} ppm)!
          </Text>
        </View>
      )}

      <Text style={styles.title}>Smart Home</Text>
      {/* Senzori */}
      <Text style={styles.section}>Senzori</Text>
      <View style={styles.row}>
        <StatusCard
          icon={<Ionicons name="thermometer" size={38} color={colors.primary} />}
          label="Temperatură"
          value={`${data.sensors.esp32_temperature} °C`}
          color={colors.primary}
        />
        <StatusCard
          icon={<Ionicons name="water" size={38} color={colors.secondary} />}
          label="Umiditate"
          value={`${data.sensors.esp32_humidity} %`}
          color={colors.secondary}
        />
        <StatusCard
          icon={<Feather name="cloud" size={38} color={gazVal > GAZ_LIMIT ? colors.danger : colors.accent} />}
          label="Gaz"
          value={`${gazVal} ppm`}
          color={gazVal > GAZ_LIMIT ? colors.danger : colors.accent}
        />
      </View>

      {/* Device-uri */}
      <Text style={styles.section}>Device-uri</Text>
      <View style={styles.row}>
        <StatusCard
          icon={<Feather name="activity" size={36} color={data.devices.kitchen === "on" ? colors.secondary : colors.textSecondary} />}
          label="Bucătărie"
          value={data.devices.kitchen === "on" ? "Pornit" : "Oprit"}
          color={data.devices.kitchen === "on" ? colors.secondary : colors.textSecondary}
        />
        <StatusCard
          icon={<Feather name="activity" size={36} color={data.devices.living === "on" ? colors.accent : colors.textSecondary} />}
          label="Living"
          value={data.devices.living === "on" ? "Pornit" : "Oprit"}
          color={data.devices.living === "on" ? colors.accent : colors.textSecondary}
        />
        <StatusCard
          icon={<Feather name="activity" size={36} color={data.devices.bedroom === "on" ? "#ba42f6" : colors.textSecondary} />}
          label="Dormitor"
          value={data.devices.bedroom === "on" ? "Pornit" : "Oprit"}
          color={data.devices.bedroom === "on" ? "#ba42f6" : colors.textSecondary}
        />
        <StatusCard
          icon={<Feather name="wind" size={36} color={data.devices.cooler === "on" ? colors.secondary : colors.textSecondary} />}
          label="Cooler"
          value={data.devices.cooler === "on" ? "Pornit" : "Oprit"}
          color={data.devices.cooler === "on" ? colors.secondary : colors.textSecondary}
        />
      </View>
    </ScrollView>
  );
}

// Card status pentru senzori/device
function StatusCard({ icon, label, value, color }:any) {
  return (
    <View style={[styles.card, { borderColor: color, shadowColor: color + '55' }]}>
      {icon}
      <Text style={[styles.cardLabel, { color }]}>{label}</Text>
      <Text style={styles.cardValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flexGrow: 1, alignItems: "center", paddingVertical: spacing.lg, backgroundColor: colors.background },
  text: { fontSize: font.size.lg, color: colors.text },
  title: { fontSize: font.size.xl, fontWeight: "bold", marginVertical: spacing.md, color: colors.primary, letterSpacing: 0.2 },
  section: { marginTop: spacing.lg, marginBottom: spacing.sm, fontWeight: "bold", fontSize: font.size.lg, color: colors.text },
  row: { flexDirection: "row", flexWrap: "wrap", justifyContent: "center", marginBottom: spacing.sm },
  card: {
    backgroundColor: colors.card,
    borderRadius: radii.md,
    borderWidth: 2,
    margin: spacing.sm,
    paddingVertical: spacing.md,
    paddingHorizontal: spacing.md,
    minWidth: 112,
    alignItems: "center",
    shadowOpacity: 0.13,
    shadowRadius: 10,
    shadowOffset: { width: 0, height: 6 },
    elevation: 4,
  },
  cardLabel: { fontSize: font.size.sm, marginTop: 6, fontWeight: "bold" },
  cardValue: { fontSize: font.size.md, fontWeight: "600", marginTop: 2, color: colors.textSecondary },
  alertBox: {
    backgroundColor: colors.danger, borderRadius: radii.md, padding: spacing.md,
    flexDirection: "row", alignItems: "center", marginBottom: spacing.md,
    shadowColor: colors.danger, shadowOpacity: 0.18, shadowRadius: 10, elevation: 6
  },
  alertText: { color: colors.card, fontWeight: "700", fontSize: font.size.sm, marginLeft: 10 },
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