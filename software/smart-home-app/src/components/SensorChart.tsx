import React from "react";
import { View, Dimensions, StyleSheet } from "react-native";
import { LineChart } from "react-native-chart-kit";
import { colors, spacing, radii } from '../utils/theme';

interface Props {
  title: string;
  data: { timestamp: string; value: number }[];
  color?: string;
  unit?: string;
}

export default function SensorChart({ title, data, color = "#0099ff", unit = "" }: Props) {
  if (!data || data.length === 0) return null;

  const labels = data
    .slice(-10)
    .map((d) => {
      const date = new Date(d.timestamp);
      return `${date.getHours()}:${String(date.getMinutes()).padStart(2, "0")}`;
    });

  const values = data.slice(-10).map((d) => Number(d.value));

  return (
    <View style={styles.container}>
      <LineChart
        data={{
          labels,
          datasets: [{ data: values }]
        }}
        width={Dimensions.get("window").width - 32}
        height={220}
        yAxisSuffix={unit}
        yAxisInterval={1}
        chartConfig={{
          backgroundColor: "#fff",
          backgroundGradientFrom: "#eaf6fd",
          backgroundGradientTo: "#fff",
          decimalPlaces: 1,
          color: () => color,
          labelColor: () => "#444",
          propsForDots: { r: "5", strokeWidth: "2", stroke: color }
        }}
        bezier
        style={styles.chart}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    marginVertical: spacing.md,
    borderRadius: radii.md,
    backgroundColor: colors.card,
    shadowColor: colors.shadow,
    shadowOpacity: 0.13,
    shadowRadius: 10,
    elevation: 4,
    borderWidth: 1.5,
    borderColor: colors.border,
    paddingTop: spacing.sm,
  },
  chart: { borderRadius: radii.md },
});