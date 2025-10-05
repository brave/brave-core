export type Point = [number, number];

export interface WeatherMeasurementWind {
  deg: number;
  speed: number;
  gust?: number | null;
  beaufortScale?: number | null;
}

export type ThreeHourPrediction = {
  utcTs: number;
  dateStr: string;
  hour: string;
  temp: {
    metric: number | null;
    us: number | null;
  };
  rain: number | null;
  wind: WeatherMeasurementWind | null;
};

// Weather API types based on the original structure
export interface Location {
  id: number;
  name: string;
  country: string;
  state?: string | null;
  coords: Coordinates;
  population: number;
  sunrise: number;
  sunset: number;
  tzoffset: number;
}

export interface Coordinates {
  lat: number;
  lon: number;
}

export interface CurrentWeather {
  ts: number;
  sunrise: number;
  sunset: number;
  temp: number;
  feels_like: number;
  pressure: number;
  humidity: number;
  dew_point: number;
  uvi: number;
  clouds: number;
  visibility: number;
  weather: WeatherSnapshot;
  wind: WindSnapshot;
}

export interface WeatherSnapshot {
  id: number;
  main: string;
  description: string;
  icon: string;
}

export interface WindSnapshot {
  speed: number;
  deg: number;
  gust?: number | null;
}

export interface TemperatureDailySummary {
  day: number;
  min: number;
  max: number;
  night: number;
  evening: number;
  morning: number;
}

export interface WeatherSummaryDay {
  ts: number;
  date_i18n?: string | null;
  sunrise: number;
  sunset: number;
  moonrise: number;
  moonset: number;
  temperature: TemperatureDailySummary;
  pressure: number;
  humidity: number;
  weather: WeatherSnapshot;
  wind: WindSnapshot;
  pop: number;
  feels_like: {
    day: number;
    night: number;
    evening: number;
    morning: number;
  };
  dew_point: number;
  uvi: number;
  clouds: number;
}

export interface WeatherSummary3Hours {
  ts: number;
  temperature: {
    temp: number;
    feels_like: number;
    min: number;
    max: number;
  };
  pressure: number;
  weather: WeatherSnapshot;
  wind: WindSnapshot;
  humidity: number;
  clouds: number;
  pop: number;
  visibility?: number | null;
  rain?: number | null;
}

export interface WeatherAlert {
  sender: string;
  event: string;
  ts_start: number;
  ts_end: number;
  start_relative_i18n?: string | null;
  description: string;
  tags: string[];
}

export interface WeatherResult {
  location: Location;
  current_time_iso: string;
  current_weather: CurrentWeather;
  daily: WeatherSummaryDay[];
  hours3: WeatherSummary3Hours[];
  alerts: WeatherAlert[];
}

export interface RichDataProvider {
  name: string;
  url: string;
}

export type ChartType = 'rain' | 'wind' | 'temp';

export enum Theme {
  light = 'light',
  dark = 'dark',
  system = '',
}
