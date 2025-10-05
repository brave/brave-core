// Stubs for external integrations that will be connected later
import * as React from 'react';

// Translation system - these will be replaced by your app's i18n system
export type Translation = {
  [key: string]: string;
};

export function translate(
  translations: Translation,
  phrase: keyof Translation,
  bindings?: { [key: string]: string | number },
): string {
  let translatedPhrase = translations[phrase] || phrase.toString();

  if (bindings) {
    for (const [name, value] of Object.entries(bindings)) {
      translatedPhrase = translatedPhrase.replace(`{${name}}`, `${value}`);
    }
  }

  return translatedPhrase;
}

// Default translations - replace with your app's translation system
export const defaultTranslations: Translation = {
  // General
  'Temperature': 'Temperature',
  'Precipitation': 'Precipitation',
  'Wind': 'Wind',
  'Humidity': 'Humidity',
  'Now': 'Now',
  'High {temperature}': 'High {temperature}',
  'Low {temperature}': 'Low {temperature}',
  'Data from': 'Data from',
  'Show more': 'Show more',
  'Show less': 'Show less',
  'Extreme weather advisory': 'Extreme weather advisory',
  'detailed-weather-data-not-available-for-{date}': 'Detailed weather data not available for {date}',

  // Units
  'KpH': 'km/h',
  'MpH': 'mph',

  // Days of week
  'Sun': 'Sun',
  'Mon': 'Mon',
  'Tue': 'Tue',
  'Wed': 'Wed',
  'Thu': 'Thu',
  'Fri': 'Fri',
  'Sat': 'Sat',
};

// Portal system stub - replace with your app's portal implementation
export const createPortal = (children: React.ReactNode, container: Element | null) => {
  if (typeof window !== 'undefined' && container) {
    // This is a stub - in React you would use ReactDOM.createPortal
    return children;
  }
  return children;
};

// Units system
export const Units = {
  metric: {
    value: 'metric',
    label: 'Metric',
    desc: 'Kilograms, meters, celsius',
  },
  us: {
    value: 'us',
    label: 'Imperial',
    desc: 'Pounds, feet, fahrenheit'
  },
} as const;

// Node IDs for portals
export const NodeId = {
  fullscreenChatPortalTarget: 'fullscreen-chat-portal-target',
};

// Mock page data - replace with your app's global state
export interface PageData {
  units: 'metric' | 'us';
  theme: 'light' | 'dark' | '';
  language: string;
  isMobile: boolean;
  i18n: Translation;
  hrefTarget: string;
}

export const mockPageData: PageData = {
  units: 'metric',
  theme: 'light',
  language: 'en-US',
  isMobile: false,
  i18n: defaultTranslations,
  hrefTarget: '_blank',
};

// Form submission stubs - replace with your app's form handling
export type SubmitFunction = (options: { formData: FormData }) => (() => void) | void;

export const enhance = (submitFunction?: SubmitFunction) => {
  return {
    onSubmit: (event: React.FormEvent<HTMLFormElement>) => {
      if (submitFunction) {
        event.preventDefault();
        const formData = new FormData(event.currentTarget);
        const callback = submitFunction({ formData });
        if (callback) {
          callback();
        }
      }
    }
  };
};

// Navigation stub - replace with your router's invalidation system
export const invalidate = (route: string) => {
  console.log('Invalidate route:', route);
  // This would typically trigger a data refresh in your router
};

// LLM integration stubs - replace with your app's AI features
export const triggerLLMSnippet = () => {
  console.log('LLM snippet triggered');
  // This would integrate with your app's AI summarization feature
};

// Event system stubs - replace with your app's event system
export const addCustomEventListener = (
  event: string,
  callback: () => void
) => {
  if (typeof window !== 'undefined') {
    window.addEventListener(event, callback);
    return () => window.removeEventListener(event, callback);
  }
  return () => {};
};

// Mock components that would be provided by your app
export function MockInterpunctSpacer () { return (
<span> • </span>
)
}

export function MockRichFooter (props: React.PropsWithChildren<{
  optionSet?: string;
}>) { return (
  <div className="rich-footer">
    {props.children}
  </div>
);
}

// Icons stubs - replace with your app's icon system
export function InfoOutlineIcon () { return (
  <svg className="icon" viewBox="0 0 24 24" fill="currentColor">
    <path d="M11,9H13V7H11M12,20C7.59,20 4,16.41 4,12C4,7.59 7.59,4 12,4C16.41,4 20,7.59 20,12C20,16.41 16.41,20 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M11,17H13V11H11V17Z"/>
  </svg>
);
}

export function ClockIcon () { return (
  <svg className="icon" viewBox="0 0 24 24" fill="currentColor">
    <path d="M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M16.2,16.2L11,13V7H12.5V12.2L17,14.7L16.2,16.2Z"/>
  </svg>
);
}
