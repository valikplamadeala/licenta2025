// src/components/IntervalSelector.tsx
import React from "react";
import { View, TouchableOpacity, Text, StyleSheet } from "react-native";
import { colors, spacing, font, radii } from '../utils/theme';

export default function IntervalSelector({ value, onChange, options } : any) {
  return (
    <View style={styles.row}>
      {options.map((opt: { key: React.Key | null | undefined; label: string | number | bigint | boolean | React.ReactElement<unknown, string | React.JSXElementConstructor<any>> | Iterable<React.ReactNode> | React.ReactPortal | Promise<string | number | bigint | boolean | React.ReactPortal | React.ReactElement<unknown, string | React.JSXElementConstructor<any>> | Iterable<React.ReactNode> | null | undefined> | null | undefined; }) => (
        <TouchableOpacity
          key={opt.key}
          onPress={() => onChange(opt.key)}
          style={[styles.btn, value === opt.key && styles.btnActive]}
        >
          <Text style={[styles.label, value === opt.key && styles.labelActive]}>
            {opt.label}
          </Text>
        </TouchableOpacity>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', justifyContent: 'center', marginBottom: spacing.xs },
  btn: {
    paddingVertical: spacing.xs,
    paddingHorizontal: spacing.md,
    borderRadius: radii.lg,
    backgroundColor: colors.background,
    marginHorizontal: spacing.xs,
    borderWidth: 1.5,
    borderColor: colors.border,
  },
  btnActive: { backgroundColor: colors.primary, borderColor: colors.primary },
  label: { color: colors.textSecondary, fontWeight: '600', fontSize: font.size.sm },
  labelActive: { color: colors.card },
});