// src/api/smartHomeApi.ts
import axios from "axios";

const BASE_URL = "http://192.168.3.86:8080/api";

export const getStatus = () => axios.get(`${BASE_URL}/status`);
export const getTemperature = () => axios.get(`${BASE_URL}/temperature`);
export const getHumidity = () => axios.get(`${BASE_URL}/humidity`);
export const getGas = () => axios.get(`${BASE_URL}/gaz`);



export const getCoolerEnergyStats = (interval: any) =>
  axios.get(`${BASE_URL}/cooler/energy?interval=${interval}`);

export const setDeviceState = (device: string, state: "on" | "off") =>
  axios.post(`${BASE_URL}/devices/${device}`, { state });

export const getDeviceState = (device: string) =>
  axios.get(`${BASE_URL}/devices/${device}`);
export const getSmartHomeStatus = () =>
  axios.get(`${BASE_URL}/status`);

export const getSensorStats = (sensor: string, interval: any) =>
  axios.get(`${BASE_URL}/${sensor}/stats?interval=${interval}`);

export const getCoolerUsage = (interval: any) =>
  axios.get(`${BASE_URL}/cooler/usage?interval=${interval}`);

export const getTemperatureHistory = () =>
  axios.get(`${BASE_URL}/temperature/history`);
export const getHumidityHistory = () =>
  axios.get(`${BASE_URL}/humidity/history`);
export const getGasHistory = () =>
  axios.get(`${BASE_URL}/gaz/history`);