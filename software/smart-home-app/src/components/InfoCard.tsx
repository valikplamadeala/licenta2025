// src/components/InfoCard.tsx
import React from "react";
import { View, Text, StyleSheet } from "react-native";
import { MaterialCommunityIcons } from "@expo/vector-icons";
import { colors, spacing, font, radii } from '../utils/theme';

interface Props {
  title: string;
  icon: keyof typeof MaterialCommunityIcons.glyphMap;
  value: string;
  subtext?: string;
  color?: string;
  graph?: React.ReactNode; // Sparkline sau progres bar
  alert?: boolean;
}

export default function InfoCard({
  title, icon, value, subtext, color = colors.card, graph, alert
}: Props) {
  return (
    <View style={[
      styles.card,
      { backgroundColor: color },
      alert && styles.alert
    ]}>
      <MaterialCommunityIcons name={icon} size={38} color={alert ? colors.danger : colors.primary} />
      <View style={{ flex: 1, marginLeft: spacing.md }}>
        <Text style={styles.title}>{title}</Text>
        <Text style={[styles.value, alert && { color: colors.danger }]}>{value}</Text>
        {subtext ? <Text style={styles.subtext}>{subtext}</Text> : null}
        {graph ? <View style={{ marginTop: 4 }}>{graph}</View> : null}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    width: "96%",
    borderRadius: radii.md,
    padding: spacing.md,
    marginVertical: spacing.sm,
    flexDirection: "row",
    alignItems: "center",
    shadowColor: colors.shadow,
    shadowOpacity: 0.13,
    shadowRadius: 10,
    elevation: 4,
    borderWidth: 1.5,
    borderColor: colors.border,
    backgroundColor: colors.card,
  },
  alert: {
    borderWidth: 2,
    borderColor: colors.danger,
    backgroundColor: "#fff1f3"
  },
  title: { fontSize: font.size.md, fontWeight: "bold", color: colors.text },
  value: { fontSize: font.size.lg, fontWeight: "bold", color: colors.text, marginTop: 2 },
  subtext: { color: colors.textSecondary, fontSize: font.size.xs },
});