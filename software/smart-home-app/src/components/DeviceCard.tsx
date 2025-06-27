import React from "react";
import { View, Text, StyleSheet } from "react-native";
import { Switch } from "react-native-paper";
import { MaterialCommunityIcons } from "@expo/vector-icons";
import { LinearGradient } from 'expo-linear-gradient';
import { colors, spacing, radii, font } from '../utils/theme';

interface Props {
  name: string;
  icon: keyof typeof MaterialCommunityIcons.glyphMap;
  isOn: boolean;
  onToggle: (value: boolean) => void;
}

export default function DeviceCard({ name, icon, isOn, onToggle }: Props) {
  return (
    isOn ? (
      <LinearGradient
        colors={colors.cardActiveGradient}
        style={[styles.card, styles.cardOn]}
        start={{ x: 0, y: 0 }}
        end={{ x: 1, y: 1 }}
      >
        <MaterialCommunityIcons
          name={icon}
          size={40}
          color={colors.primary}
          style={styles.icon}
        />
        <Text style={styles.label}>{name}</Text>
        <Switch
          value={isOn}
          onValueChange={onToggle}
          color={colors.primary}
        />
      </LinearGradient>
    ) : (
      <View style={styles.card}>
        <MaterialCommunityIcons
          name={icon}
          size={40}
          color={colors.textSecondary}
          style={styles.icon}
        />
        <Text style={styles.label}>{name}</Text>
        <Switch
          value={isOn}
          onValueChange={onToggle}
          color={colors.primary}
        />
      </View>
    )
  );
}

const styles = StyleSheet.create({
  card: {
    width: '95%',
    backgroundColor: colors.card,
    borderRadius: radii.lg,
    flexDirection: 'row',
    alignItems: 'center',
    marginVertical: spacing.sm,
    padding: spacing.md,
    shadowColor: colors.shadow,
    shadowOpacity: 0.13,
    shadowRadius: 12,
    shadowOffset: { width: 0, height: 6 },
    elevation: 6,
  },
  cardOn: {
    backgroundColor: undefined,
  },
  icon: {
    marginRight: spacing.md,
  },
  label: {
    flex: 1,
    fontSize: font.size.lg,
    fontWeight: 'bold',
    color: colors.text,
    fontFamily: font.family,
    letterSpacing: 0.2,
  },
});