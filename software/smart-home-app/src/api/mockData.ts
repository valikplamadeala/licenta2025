export const mockStatus = {
  sensors: {
    esp32_temperature: 22.5,
    esp32_humidity: 48,
    esp32_gaz: 120,
  },
  devices: {
    kitchen: "on",
    living: "off",
    bedroom: "on",
    cooler: "off",
  },
};

export const mockTemperatureHistory = {
  history: Array.from({ length: 10 }, (_, i) => ({
    timestamp: new Date(Date.now() - (9 - i) * 3600 * 1000).toISOString(),
    value: 20 + Math.random() * 5,
  })),
};

export const mockHumidityHistory = {
  history: Array.from({ length: 10 }, (_, i) => ({
    timestamp: new Date(Date.now() - (9 - i) * 3600 * 1000).toISOString(),
    value: 40 + Math.random() * 20,
  })),
};

export const mockGasHistory = {
  history: Array.from({ length: 10 }, (_, i) => ({
    timestamp: new Date(Date.now() - (9 - i) * 3600 * 1000).toISOString(),
    value: 100 + Math.random() * 50,
  })),
};

export const mockSensorStats = {
  stats: Array.from({ length: 7 }, (_, i) => ({
    label: `Ziua ${i + 1}`,
    avg: 20 + Math.random() * 5,
    min: 18 + Math.random() * 2,
    max: 25 + Math.random() * 2,
  })),
};

export const mockCoolerUsage = {
  usage: Array.from({ length: 7 }, (_, i) => ({
    label: `Ziua ${i + 1}`,
    hours: Math.round(Math.random() * 8),
    cost: Math.round(Math.random() * 10 * 100) / 100,
  })),
}; 