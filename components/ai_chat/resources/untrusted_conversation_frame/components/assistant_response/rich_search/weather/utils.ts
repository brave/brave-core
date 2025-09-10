import { Theme } from './types';

// Date formatting utilities
export function formatDateShort(
  date: Date | string | number | null | undefined,
  uiLang?: string
): string {
  if (!date) return '';
  
  const dateObj = date instanceof Date ? date : new Date(date);
  const lang = uiLang || getUiLang();
  
  try {
    return new Intl.DateTimeFormat(lang, {
      year: '2-digit',
      month: 'numeric',
      day: 'numeric',
    }).format(dateObj);
  } catch {
    return dateObj.toLocaleDateString();
  }
}

export function formatHour(
  date: Date | string | number | null | undefined,
  uiLang?: string
): string {
  if (!date) return '';
  
  const dateObj = date instanceof Date ? date : new Date(date);
  const lang = uiLang || getUiLang();
  
  try {
    return new Intl.DateTimeFormat(lang, {
      hour: 'numeric',
    }).format(dateObj);
  } catch {
    return dateObj.toLocaleTimeString();
  }
}

export function formatHourAndMinutes(
  date: Date | string | number | null | undefined,
  uiLang?: string
): string {
  if (!date) return '';
  
  const dateObj = date instanceof Date ? date : new Date(date);
  const lang = uiLang || getUiLang();
  
  try {
    return new Intl.DateTimeFormat(lang, {
      hour: 'numeric',
      minute: '2-digit',
    }).format(dateObj);
  } catch {
    return dateObj.toLocaleTimeString();
  }
}

// Number formatting utilities
export function formatPercentShort(
  n: number | undefined | null, 
  uiLang?: string
): string {
  if (n === undefined || n === null) {
    return 'n/a';
  }
  
  const lang = uiLang || getUiLang();
  try {
    return n.toLocaleString(lang, {
      maximumFractionDigits: 0,
      minimumFractionDigits: 0,
      style: 'percent',
      signDisplay: 'never',
    });
  } catch {
    return Math.round(n * 100) + '%';
  }
}

// Theme utilities
export function getExplicitTheme(implicitTheme: Theme): Theme.dark | Theme.light {
  if (implicitTheme === Theme.system) {
    if (
      typeof window !== 'undefined' &&
      window.matchMedia('(prefers-color-scheme: dark)').matches
    ) {
      return Theme.dark;
    } else {
      return Theme.light;
    }
  }
  return implicitTheme as Theme.dark | Theme.light;
}

// Language utilities
export function getUiLang(): string {
  return (typeof document !== 'undefined' && document.documentElement.lang) || 'en-US';
}

// Error function for development
export const error = (...args: any[]) => {
  if (process.env.NODE_ENV !== 'production') {
    console.error(...args);
  }
};

// Unit conversion utilities
export const celsiusToFahrenheit = (c: number) => {
  return c * 1.8 + 32;
};

export const unitConvert = (value: number | undefined, target = 'metric', decimals = 0) => {
  if (value === undefined) {
    return 'n/a';
  }
  if (target === 'metric') {
    return value.toFixed(decimals);
  } else if (target === 'us') {
    return celsiusToFahrenheit(value).toFixed(decimals);
  }
  return value.toFixed(decimals);
};

// Wind direction utilities
export const getWindDirection = (angle: number) => {
  const directions = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
  const idx = Math.round(((angle %= 360) < 0 ? angle + 360 : angle) / 45) % 8;
  return directions[idx];
};

export const getBeaufortScale = (speed: number) => {
  // Note: speed is provided in meters/sec.
  if (speed < 0.5) {
    return 0;
  } else if (speed <= 1.5) {
    return 1;
  } else if (speed <= 3.3) {
    return 2;
  } else if (speed <= 5.5) {
    return 3;
  } else if (speed <= 7.9) {
    return 4;
  } else if (speed <= 10.7) {
    return 5;
  } else if (speed <= 13.8) {
    return 6;
  } else if (speed <= 17.1) {
    return 7;
  } else if (speed <= 20.7) {
    return 8;
  } else if (speed <= 24.4) {
    return 9;
  } else if (speed <= 28.4) {
    return 10;
  } else if (speed <= 32.6) {
    return 11;
  } else {
    return 12;
  }
};