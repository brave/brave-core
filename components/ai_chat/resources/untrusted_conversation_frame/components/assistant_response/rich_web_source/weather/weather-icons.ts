// Weather icons - imported as static assets (webpack file-loader compatible)
import DarkIcon01d from './icons/weather_dark/01d.svg';
import DarkIcon01n from './icons/weather_dark/01n.svg';
import DarkIcon02d from './icons/weather_dark/02d.svg';
import DarkIcon02n from './icons/weather_dark/02n.svg';
import DarkIcon03d from './icons/weather_dark/03d.svg';
import DarkIcon03n from './icons/weather_dark/03n.svg';
import DarkIcon04d from './icons/weather_dark/04d.svg';
import DarkIcon04n from './icons/weather_dark/04n.svg';
import DarkIcon09d from './icons/weather_dark/09d.svg';
import DarkIcon09n from './icons/weather_dark/09n.svg';
import DarkIcon10d from './icons/weather_dark/10d.svg';
import DarkIcon10n from './icons/weather_dark/10n.svg';
import DarkIcon11d from './icons/weather_dark/11d.svg';
import DarkIcon11n from './icons/weather_dark/11n.svg';
import DarkIcon13d from './icons/weather_dark/13d.svg';
import DarkIcon13n from './icons/weather_dark/13n.svg';
import DarkIcon50d from './icons/weather_dark/50d.svg';
import DarkIcon50n from './icons/weather_dark/50n.svg';

import LightIcon01d from './icons/weather_light/01d.svg';
import LightIcon01n from './icons/weather_light/01n.svg';
import LightIcon02d from './icons/weather_light/02d.svg';
import LightIcon02n from './icons/weather_light/02n.svg';
import LightIcon03d from './icons/weather_light/03d.svg';
import LightIcon03n from './icons/weather_light/03n.svg';
import LightIcon04d from './icons/weather_light/04d.svg';
import LightIcon04n from './icons/weather_light/04n.svg';
import LightIcon09d from './icons/weather_light/09d.svg';
import LightIcon09n from './icons/weather_light/09n.svg';
import LightIcon10d from './icons/weather_light/10d.svg';
import LightIcon10n from './icons/weather_light/10n.svg';
import LightIcon11d from './icons/weather_light/11d.svg';
import LightIcon11n from './icons/weather_light/11n.svg';
import LightIcon13d from './icons/weather_light/13d.svg';
import LightIcon13n from './icons/weather_light/13n.svg';
import LightIcon50d from './icons/weather_light/50d.svg';
import LightIcon50n from './icons/weather_light/50n.svg';

export const darkIcons = {
  '01d': DarkIcon01d, // clear sky day
  '01n': DarkIcon01n, // clear sky night
  '02d': DarkIcon02d, // few clouds day
  '02n': DarkIcon02n, // few clouds night
  '03d': DarkIcon03d, // scattered clouds day
  '03n': DarkIcon03n, // scattered clouds night
  '04d': DarkIcon04d, // broken clouds day
  '04n': DarkIcon04n, // broken clouds night
  '09d': DarkIcon09d, // shower rain day
  '09n': DarkIcon09n, // shower rain night
  '10d': DarkIcon10d, // rain day
  '10n': DarkIcon10n, // rain night
  '11d': DarkIcon11d, // thunderstorm day
  '11n': DarkIcon11n, // thunderstorm night
  '13d': DarkIcon13d, // snow day
  '13n': DarkIcon13n, // snow night
  '50d': DarkIcon50d, // mist day
  '50n': DarkIcon50n, // mist night
} as const;

export const lightIcons = {
  '01d': LightIcon01d,
  '01n': LightIcon01n,
  '02d': LightIcon02d,
  '02n': LightIcon02n,
  '03d': LightIcon03d,
  '03n': LightIcon03n,
  '04d': LightIcon04d,
  '04n': LightIcon04n,
  '09d': LightIcon09d,
  '09n': LightIcon09n,
  '10d': LightIcon10d,
  '10n': LightIcon10n,
  '11d': LightIcon11d,
  '11n': LightIcon11n,
  '13d': LightIcon13d,
  '13n': LightIcon13n,
  '50d': LightIcon50d,
  '50n': LightIcon50n,
} as const;

export function isWeatherIcon(icon: string): icon is keyof typeof darkIcons {
  return icon in darkIcons;
}