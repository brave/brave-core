// Platform utilities for detecting user preferences from the web platform
import { Theme } from './types';

/**
 * Detect user's preferred units based on their locale/region
 * Returns 'us' for US, Liberia, and Myanmar; 'metric' for everyone else
 */
export function detectPreferredUnits(): 'metric' | 'us' {
  if (typeof navigator === 'undefined') {
    return 'metric'; // Default fallback
  }

  // Get user's locale/region preferences
  const locales = navigator.languages || [navigator.language];

  // Countries that primarily use Imperial system
  const imperialCountries = ['US', 'LR', 'MM']; // United States, Liberia, Myanmar

  for (const locale of locales) {
    // Extract country code from locale (e.g., 'en-US' -> 'US')
    const countryCode = locale.split('-')[1]?.toUpperCase();
    if (countryCode && imperialCountries.includes(countryCode)) {
      return 'us';
    }
  }

  return 'metric';
}

/**
 * Detect user's preferred theme from system preferences
 */
export function detectPreferredTheme(): Theme {
  if (typeof window === 'undefined') {
    return Theme.light; // Default fallback for SSR
  }

  if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
    return Theme.dark;
  } else {
    return Theme.light;
  }
}

/**
 * Detect user's preferred language from browser settings
 */
export function detectPreferredLanguage(): string {
  if (typeof navigator === 'undefined') {
    return 'en-US';
  }

  return navigator.language || 'en-US';
}

/**
 * Detect user's timezone
 */
export function detectTimezone(): string {
  if (typeof Intl === 'undefined' || !Intl.DateTimeFormat) {
    return 'UTC';
  }

  try {
    return Intl.DateTimeFormat().resolvedOptions().timeZone;
  } catch {
    return 'UTC';
  }
}

/**
 * Get user's preferred temperature format based on their locale
 * Some countries use Celsius but prefer Fahrenheit displays (like some parts of UK)
 */
export function detectPreferredTemperatureDisplay(): 'celsius' | 'fahrenheit' {
  if (typeof navigator === 'undefined') {
    return 'celsius';
  }

  const locale = navigator.language || 'en-US';
  const countryCode = locale.split('-')[1]?.toUpperCase();

  // Countries that commonly display temperatures in Fahrenheit
  const fahrenheitCountries = ['US', 'BS', 'BZ', 'KY', 'PW'];

  return fahrenheitCountries.includes(countryCode || '') ? 'fahrenheit' : 'celsius';
}

/**
 * Check if user prefers reduced motion (accessibility)
 */
export function detectPrefersReducedMotion(): boolean {
  if (typeof window === 'undefined') {
    return false;
  }

  return window.matchMedia('(prefers-reduced-motion: reduce)').matches;
}

/**
 * Create media query listeners for dynamic preference changes
 */
export function createThemeListener(callback: (theme: Theme) => void): () => void {
  if (typeof window === 'undefined') {
    return () => {};
  }

  const darkQuery = window.matchMedia('(prefers-color-scheme: dark)');

  const handleChange = (e: MediaQueryListEvent) => {
    callback(e.matches ? Theme.dark : Theme.light);
  };

  darkQuery.addEventListener('change', handleChange);

  return () => darkQuery.removeEventListener('change', handleChange);
}

/**
 * Create reduced motion listener
 */
export function createReducedMotionListener(callback: (prefersReduced: boolean) => void): () => void {
  if (typeof window === 'undefined') {
    return () => {};
  }

  const motionQuery = window.matchMedia('(prefers-reduced-motion: reduce)');

  const handleChange = (e: MediaQueryListEvent) => {
    callback(e.matches);
  };

  motionQuery.addEventListener('change', handleChange);

  return () => motionQuery.removeEventListener('change', handleChange);
}

/**
 * Get viewport dimensions for responsive behavior
 */
export function getViewportDimensions(): { width: number; height: number } {
  if (typeof window === 'undefined') {
    return { width: 1024, height: 768 }; // Default fallback
  }

  return {
    width: window.innerWidth,
    height: window.innerHeight,
  };
}

/**
 * Comprehensive platform detection for weather widget defaults
 */
export function detectWeatherDefaults() {
  return {
    units: detectPreferredUnits(),
    theme: detectPreferredTheme(),
    language: detectPreferredLanguage(),
    isMobile: false,
    timezone: detectTimezone(),
    temperatureDisplay: detectPreferredTemperatureDisplay(),
    prefersReducedMotion: detectPrefersReducedMotion(),
    viewport: getViewportDimensions(),
  };
}

/**
 * React hook for platform preferences with state management
 */
export function usePlatformDefaults() {
  // For SSR compatibility, return static defaults
  if (typeof window === 'undefined') {
    return {
      units: 'metric' as const,
      theme: Theme.light,
      language: 'en-US',
      isMobile: false,
      timezone: 'UTC',
      temperatureDisplay: 'celsius' as const,
      prefersReducedMotion: false,
      viewport: { width: 1024, height: 768 },
    };
  }

  // For client-side, detect platform defaults
  // Note: In a real React app, you might want to use useState/useEffect
  // to handle changes to these values over time
  return detectWeatherDefaults();
}

/**
 * Enhanced React hook with state management for dynamic updates
 * Use this version if you want to respond to system preference changes
 */
export function usePlatformDefaultsWithState() {
  // This would typically use React state management
  // For now, returning the static detection
  // In a full implementation, you'd use useState/useEffect to listen for changes

  const defaults = usePlatformDefaults();

  // Example of how you might handle dynamic updates:
  // const [theme, setTheme] = useState(defaults.theme);
  // const [units, setUnits] = useState(defaults.units);

  // useEffect(() => {
  //   const cleanup = createThemeListener(setTheme);
  //   return cleanup;
  // }, []);

  return defaults;
}
