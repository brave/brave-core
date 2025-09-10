// Weather component exports
export { default as Weather } from './weather';
export { default as WeatherAlert } from './weather_alert';
export { default as TemperatureChart } from './charts/temperature_chart';
export { default as PrecipitationChart } from './charts/precipitation_chart';
export { default as WindGraphic } from './charts/wind_graphic';
export { default as WindSvgs } from './charts/wind_svgs';
export { default as TemperatureChartGradients } from './charts/temperature_chart_gradients';

// Types
export type {
  WeatherResult as MixerWeatherResult,
  RichDataProvider as MixerDataProvider,
  WeatherAlert as WeatherAlertType,
  ThreeHourPrediction,
  ChartType,
  Point,
  Theme,
  Location,
  WeatherSummaryDay,
  WeatherSummary3Hours,
  CurrentWeather,
  WeatherSnapshot,
  WindSnapshot,
  WeatherMeasurementWind,
} from './types';

// Utilities
export {
  formatDateShort,
  formatHour,
  formatHourAndMinutes,
  formatPercentShort,
  getExplicitTheme,
  unitConvert,
  celsiusToFahrenheit,
  getWindDirection,
  getBeaufortScale,
  error,
} from './utils';

// Chart utilities
export {
  map,
  line,
  controlPoint,
  bezierCommand,
  createBezierPath,
  convertWindSpeed,
} from './charts/chart_utils';

// Weather icons
export {
  lightIcons,
  darkIcons,
  isWeatherIcon,
} from './weather-icons';

// Platform detection utilities
export {
  detectPreferredUnits,
  detectPreferredTheme,
  detectPreferredLanguage,
  detectTimezone,
  detectPreferredTemperatureDisplay,
  detectPrefersReducedMotion,
  detectWeatherDefaults,
  usePlatformDefaults,
  usePlatformDefaultsWithState,
  createThemeListener,
  createReducedMotionListener,
  getViewportDimensions,
} from './platform';

// Stubs (to be replaced with your app's implementations)
export {
  translate,
  defaultTranslations,
  Units,
  NodeId,
  mockPageData,
} from './stubs';
