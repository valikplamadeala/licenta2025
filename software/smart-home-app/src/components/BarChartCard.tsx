// src/components/BarChartCard.tsx
import React from "react";
import { View, Text, StyleSheet, Dimensions } from "react-native";
import { BarChart } from "react-native-chart-kit";
import { colors, spacing, font, radii } from '../utils/theme';

const width = Math.min(Dimensions.get("window").width * 0.96, 400);

type BarChartDataItem = {
  label: string;
  value?: number;
  avg?: number;
  min?: number;
  max?: number;
};

type BarChartCardProps = {
  data: BarChartDataItem[];
  color: string;
  unit: string;
  title: string;
};

export default function BarChartCard({ data, color, unit, title }: BarChartCardProps) {
  if (!data || data.length === 0) return null;

  return (
    <View style={styles.card}>
      <Text style={styles.title}>{title}</Text>
      <BarChart
              data={{
                  labels: data.map(d => d.label),
                  datasets: [{ data: data.map(d => Number(d.avg || d.value)) }],
              }}
              width={width}
              height={160}
              fromZero
              showValuesOnTopOfBars
              chartConfig={{
                  backgroundGradientFrom: "#fff",
                  backgroundGradientTo: "#fff",
                  fillShadowGradient: color,
                  fillShadowGradientOpacity: 1,
                  decimalPlaces: 2,
                  color: (opacity = 1) => color,
                  labelColor: () => "#444",
                  propsForBackgroundLines: { stroke: "#eee" },
                  barPercentage: 0.7,
              }}
              style={{ borderRadius: 12 }}
              withInnerLines={true}
              withHorizontalLabels={true} yAxisLabel={""} yAxisSuffix={""}      />
      <View style={styles.infoRow}>
        <Text style={styles.info}>
          Max: {
            Math.max(
              ...data
                .map(d => d.max ?? d.value)
                .filter((v): v is number => typeof v === "number")
            )
          }{unit}
        </Text>
        <Text style={styles.info}>
          Min: {
            Math.min(
              ...data
                .map(d => d.min ?? d.value)
                .filter((v): v is number => typeof v === "number")
            )
          }{unit}
        </Text>
      </View>
    </View>
  );
}
const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.card,
    borderRadius: radii.md,
    marginVertical: spacing.md,
    width: "96%",
    padding: spacing.sm,
    alignSelf: "center",
    elevation: 4,
    shadowColor: colors.shadow,
    shadowOpacity: 0.13,
    shadowRadius: 10,
    borderWidth: 1.5,
    borderColor: colors.border,
  },
  title: { fontSize: font.size.md, fontWeight: "bold", marginBottom: spacing.xs, color: colors.primary },
  infoRow: { flexDirection: "row", justifyContent: "space-between", marginTop: spacing.xs },
  info: { fontSize: font.size.xs, color: colors.textSecondary },
});