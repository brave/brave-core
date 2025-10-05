import * as React from 'react';
import {
  WeatherResult,
  RichDataProvider,
  ChartType,
  Theme,
  ThreeHourPrediction
} from './types';
import {
  formatDateShort,
  formatHour,
  formatHourAndMinutes,
  formatPercentShort,
  getExplicitTheme,
  unitConvert,
  error
} from './utils';
import { usePlatformDefaults } from './platform';
import { lightIcons, darkIcons, isWeatherIcon } from './weather-icons';
import {
  translate,
  defaultTranslations,
  MockInterpunctSpacer,
  MockRichFooter,
  Units,
  enhance,
  invalidate,
  addCustomEventListener
} from './stubs';
import TemperatureChart from './charts/temperature_chart';
import PrecipitationChart from './charts/precipitation_chart';
import WindGraphic from './charts/wind_graphic';
import WeatherAlert from './weather_alert';
import styles from './weather.module.scss';

interface WeatherProps {
  data: WeatherResult;           // Weather data from API (required)
  provider?: RichDataProvider | null | undefined;       // Data source attribution
  disableLLM?: boolean;               // Hide AI features
  theme?: Theme;                      // Optional - will auto-detect from system
  units?: 'us' | 'metric';            // Optional - will auto-detect from locale
  language?: string;                  // Optional - will auto-detect from browser
  isMobile?: boolean;                 // Optional - will auto-detect from user agent
  translations?: typeof defaultTranslations;
  showLLM?: boolean;
}

type UnitButton = { label: string; unit: 'us' | 'metric' };

const Weather: React.FC<WeatherProps> = ({
  data,
  provider,
  disableLLM = false,
  theme: propTheme,
  units: propUnits,
  language: propLanguage,
  isMobile: propIsMobile,
  translations = defaultTranslations,
  showLLM = false,
}: WeatherProps) => {
  // Auto-detect platform defaults for missing props
  const platformDefaults = usePlatformDefaults();

  // Use props if provided, otherwise fall back to platform detection
  const detectedTheme = propTheme ?? platformDefaults.theme;
  const detectedLanguage = propLanguage ?? platformDefaults.language;
  const detectedIsMobile = propIsMobile ?? platformDefaults.isMobile;
  const initialUnits = propUnits ?? platformDefaults.units;
  // State management
  const [units, setUnits] = React.useState<'us' | 'metric'>(initialUnits);
  const [chosenDay, setChosenDay] = React.useState(0);
  const [activeChartView, setActiveChartView] = React.useState<ChartType>('temp');
  const [showLLMSnippet, setShowLLMSnippet] = React.useState(showLLM);
  const [activeTransition, setActiveTransition] = React.useState<{
    type: 'nav' | 'day';
    key: string;
  } | null>(null);

  const LLMSnippetRef = React.useRef<HTMLDivElement>(null);

  // Calculate timezone info
  const clientDate = new Date();
  const clientUTCOffset = clientDate.getTimezoneOffset() * 60;

  // Get theme-specific icons
  const explicitTheme = getExplicitTheme(detectedTheme);
  const themeIconSet = explicitTheme === Theme.light ? lightIcons : darkIcons;

  // Unit buttons configuration
  const unitButtons: [UnitButton, UnitButton] = [
    { label: 'F', unit: 'us' },
    { label: 'C', unit: 'metric' },
  ];

  if (units === 'metric') {
    unitButtons.reverse();
  }

  // Date/time processing utility
  const getDateFromTs = (utcTs: number) => {
    const targetCityOffsetFromUTC = data.location.tzoffset;
    const offset = targetCityOffsetFromUTC + clientUTCOffset;

    const dt = new Date(0);
    dt.setSeconds(utcTs + offset);

    return {
      utcTs,
      dateStr: formatDateShort(dt, detectedLanguage),
      hour: formatHour(dt, detectedLanguage),
    };
  };

  // Process 3h predictions
  const all3hpredictions: ThreeHourPrediction[] = React.useMemo(() => {
    const predictions: ThreeHourPrediction[] = [];

    if (data.hours3?.length) {
      for (const pred of data.hours3) {
        predictions.push({
          ...getDateFromTs(pred.ts),
          temp: {
            metric: pred.temperature.temp,
            us: Number(unitConvert(pred.temperature.temp, 'us')),
          },
          rain: pred.pop,
          wind: pred.wind,
        });
      }
    } else {
      error('3h predictions not available', data.location.name);
    }

    return predictions;
  }, [data.hours3, data.location.name]);

  // Get wind speed for a specific day
  const getWindSpeed = (day: number, unitsType: 'us' | 'metric') => {
    const windSpeed = day === 0 ? data.current_weather.wind.speed : data.daily[day]?.wind.speed;
    if (!windSpeed) {
      return 'n/a';
    }
    const mpsToKph = (speed: number) => speed * 3.6;
    const mpsToMph = (speed: number) => speed * 2.237;
    const converted = unitsType === 'metric' ? mpsToKph(windSpeed) : mpsToMph(windSpeed);
    return converted.toFixed(2);
  };

  // Get predictions for a chosen day
  const getPredictions = (chosenDayIndex: number): ThreeHourPrediction[] => {
    if (chosenDayIndex === 0) {
      return all3hpredictions.slice(0, 8);
    }

    const predWindow: ThreeHourPrediction[] = [];
    const dt = data.daily[chosenDayIndex]?.ts;

    if (dt) {
      const { dateStr } = getDateFromTs(dt);

      for (const pred of all3hpredictions) {
        if (pred.dateStr === dateStr) {
          predWindow.push(pred);
        }
      }
    }

    if (!predWindow.length) {
      return [];
    }

    // Fill the rest of the day with empty predictions
    let previousPred = predWindow[predWindow.length - 1];
    if (!previousPred) {
      return [];
    }

    const HOUR = 60 * 60;
    while (predWindow.length < 8) {
      const nextPrediction: ThreeHourPrediction = {
        ...getDateFromTs(previousPred.utcTs + 3 * HOUR),
        temp: { metric: null, us: null },
        rain: null,
        wind: null,
      };
      predWindow.push(nextPrediction);
      previousPred = nextPrediction;
    }

    return predWindow;
  };

  // Chart views configuration
  const chartViews: [ChartType, string][] = [
    ['temp', translate(translations, 'Temperature')],
    ['rain', translate(translations, 'Precipitation')],
    ['wind', translate(translations, 'Wind')],
  ];

  // Current time processing
  const [currentHour, setCurrentHour] = React.useState('');

  React.useEffect(() => {
    try {
      const timeInLocation = new Date(data.current_time_iso);
      setCurrentHour(formatHourAndMinutes(timeInLocation, detectedLanguage));
    } catch {
      // noop
    }
  }, [data.current_time_iso, detectedLanguage]);

  // Days of week processing
  const daysOfWeekKeys = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
  const daysOfWeek = daysOfWeekKeys.map((x) => translate(translations, x));

  const getDateRepr = (epoch: number) => {
    const dt = new Date(0);
    dt.setSeconds(epoch);
    return daysOfWeek[dt.getDay()];
  };

  // Computed values
  const currentDay = data.daily[chosenDay];
  const temperature = chosenDay === 0 ? data.current_weather.temp : currentDay?.temperature.max;
  const predictions = getPredictions(chosenDay);
  const mainIcon = chosenDay === 0 ? data.current_weather.weather.icon : data.daily[chosenDay]?.weather?.icon;
  const weatherDesc = chosenDay === 0 ? data.current_weather.weather.description : data.daily[chosenDay]?.weather?.description;
  const humidity = chosenDay === 0 ? data.current_weather.humidity : data.daily[chosenDay]?.humidity;
  const precipitation = data.daily[chosenDay]?.pop;
  const wind = getWindSpeed(chosenDay, units);
  const speedLabel = units === 'metric' ? translate(translations, 'KpH') : translate(translations, 'MpH');

  // Event handlers
  const handleDaySelect = (idx: number) => {
    if (idx !== chosenDay) {
      setActiveTransition({ type: 'day', key: 'active-day' });
      setChosenDay(idx);

      // Remove transition after animation
      setTimeout(() => setActiveTransition(null), 250);
    }
  };

  const handleChartActiveViewChange = (activeView: ChartType) => {
    if (activeView !== activeChartView) {
      setActiveTransition({ type: 'nav', key: 'active-nav-tab' });
      setActiveChartView(activeView);

      // Remove transition after animation
      setTimeout(() => setActiveTransition(null), 250);
    }
  };

  const handleUnitsSubmit = (formData: FormData) => {
    const value = formData.get('value') as string;
    if (value === Units.us.value) {
      setUnits(Units.us.value);
    } else if (value === Units.metric.value) {
      setUnits(Units.metric.value);
    }

    return () => invalidate('root:layout');
  };

  const triggerLLM = () => {
    setShowLLMSnippet(true);
  };

  const triggerLLMAutoScroll = () => {
    triggerLLM();
    setTimeout(() => {
      LLMSnippetRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, 0);
  };

  // LLM event listeners and theme change detection
  React.useEffect(() => {
    if (showLLM) {
      triggerLLM();
    }

    return addCustomEventListener('brave:chatllm:summarize', triggerLLMAutoScroll);
  }, [showLLM]);

  // Listen for system theme changes if no explicit theme is provided
  React.useEffect(() => {
    if (propTheme !== undefined) return; // Don't override explicit theme

    // Note: In a real implementation, you might want to use a theme context
    // or state management to handle theme changes more efficiently

    return () => {}; // Cleanup if needed
  }, [propTheme]);

  const enhancedFormProps = enhance(({ formData }) => handleUnitsSubmit(formData));

  return (
    <div className={styles.richWeatherContent} data-theme={explicitTheme}>
      <header>
        <div className={`${styles.locationHeading} ${detectedIsMobile ? styles.mobile : ''}`}>
          {data.location.name},
          {data.location.state && ` ${data.location.state},`}
          {' '}{data.location.country}
        </div>
        <div className={`${styles.weatherInfo} ${detectedIsMobile ? styles.mobile : ''}`}>
          <span className={styles.weatherDesc}>{weatherDesc}</span>
          <MockInterpunctSpacer />
          {data.daily[chosenDay]?.date_i18n}
          {chosenDay === 0 && (
            <span className={styles.currentHour}>, {currentHour}</span>
          )}
        </div>
      </header>

      <div className={styles.body}>
        <div className={styles.left}>
          <div className={styles.weather}>
            {mainIcon && isWeatherIcon(mainIcon) && (
              <img
                className={`${styles.weatherIcon} ${styles.wideOnly}`}
                src={themeIconSet[mainIcon]}
                alt={data.current_weather.weather.main}
              />
            )}

            <div className={styles.temps}>
              <div className={styles.tempsHeader}>
                {mainIcon && isWeatherIcon(mainIcon) && (
                  <img
                    className={`${styles.weatherIcon} ${styles.narrowOnly}`}
                    src={themeIconSet[mainIcon]}
                    alt={data.current_weather.weather.main}
                  />
                )}
                <div className={`${styles.temperatureDisplay} ${detectedIsMobile ? styles.mobile : ''} ${styles.tempsCurrent}`}>
                  {unitConvert(temperature, units, 0)}°
                </div>
                <div className={styles.unitSwitch}>
                  <form method="POST" action="/settings" {...enhancedFormProps}>
                    <button
                      type="submit"
                      className={`${styles.unitButton} ${units === unitButtons[0].unit ? styles.active : ''}`}
                      disabled={units === unitButtons[0].unit}
                    >
                      {unitButtons[0].label}
                    </button>
                    <input type="hidden" name="name" value="units" />
                    <input type="hidden" name="value" value={Units[unitButtons[0].unit].value} />
                  </form>
                  <form method="POST" action="/settings" {...enhancedFormProps}>
                    <button
                      type="submit"
                      className={`${styles.unitButton} ${units === unitButtons[1].unit ? styles.active : ''}`}
                      disabled={units === unitButtons[1].unit}
                    >
                      {unitButtons[1].label}
                    </button>
                    <input type="hidden" name="name" value="units" />
                    <input type="hidden" name="value" value={Units[unitButtons[1].unit].value} />
                  </form>
                </div>
              </div>
              <div className={styles.divider}></div>
              <div className={`${styles.tempsHighLow} ${styles.temperatureRange}`}>
                {translate(translations, 'High {temperature}', {
                  temperature: `${unitConvert(currentDay?.temperature.max, units, 0)}°`,
                })}
                <MockInterpunctSpacer />
                {translate(translations, 'Low {temperature}', {
                  temperature: `${unitConvert(currentDay?.temperature.min, units, 0)}°`,
                })}
              </div>
            </div>
          </div>

          <div className={`${styles.table} ${styles.weatherTable}`}>
            <div className={styles.row}>
              <span>{translate(translations, 'Humidity')}</span>
              {humidity !== undefined ? (
                <span>{formatPercentShort(humidity / 100, detectedLanguage)}</span>
              ) : (
                'n/a'
              )}
            </div>
            <div className={styles.row}>
              <span>{translate(translations, 'Precipitation')}</span>
              <span>{formatPercentShort(precipitation, detectedLanguage)}</span>
            </div>
            <div className={styles.row}>
              <span>{translate(translations, 'Wind')}</span>
              <span>{wind} {speedLabel}</span>
            </div>
          </div>
        </div>

        <div className={styles.right}>
          <nav>
            {chartViews.map(([name, label]) => (
              <button
                key={name}
                className={`${styles.chartNavButton} ${name === activeChartView ? styles.active : ''}`}
                disabled={name === activeChartView}
                onClick={() => handleChartActiveViewChange(name)}
              >
                {name === activeChartView && activeTransition?.type === 'nav' && (
                  <div className={`${styles.activeBg} ${styles.slideTransition}`}></div>
                )}
                <div className={styles.content}>
                  {label}
                </div>
              </button>
            ))}
          </nav>

          <div className={`${styles.chart} noscrollbar`}>
            {activeChartView === 'temp' && (
              <TemperatureChart
                predictions={predictions}
                units={units}
                chosenDay={chosenDay}
                dti18n={data.daily[chosenDay]?.date_i18n ?? ''}
                language={detectedLanguage}
                translations={translations}
              />
            )}
            {activeChartView === 'rain' && (
              <PrecipitationChart
                key={chosenDay}
                predictions={predictions}
                chosenDay={chosenDay}
                dti18n={data.daily[chosenDay]?.date_i18n ?? ''}
                language={detectedLanguage}
                translations={translations}
              />
            )}
            {activeChartView === 'wind' && (
              <WindGraphic
                predictions={predictions}
                units={units}
                speedLabel={speedLabel}
                theme={explicitTheme}
                chosenDay={chosenDay}
                dti18n={data.daily[chosenDay]?.date_i18n ?? ''}
                language={detectedLanguage}
                translations={translations}
              />
            )}
          </div>
        </div>
      </div>

      <div className={`${styles.dayButtons} noscrollbar`}>
        {data.daily.map((day, idx) => (
          <button
            key={idx}
            className={`${styles.dayButton} ${styles.dayButtonText} ${
              idx === chosenDay ? styles.active : styles.inactive
            } ${idx === chosenDay ? styles.active : ''}`}
            onClick={() => handleDaySelect(idx)}
          >
            {idx === chosenDay && activeTransition?.type === 'day' && (
              <div className={`${styles.activeBg} ${styles.slideTransition}`}></div>
            )}
            <div className={styles.content}>
              <div>{getDateRepr(day.ts)}</div>
              {day.weather.icon && isWeatherIcon(day.weather.icon) && (
                <img className={styles.dayIcon} src={themeIconSet[day.weather.icon]} alt={day.weather.main} />
              )}
              <div>
                {unitConvert(day.temperature.max, units, 0)}°{' '}
                {unitConvert(day.temperature.min, units, 0)}°
              </div>
            </div>
          </button>
        ))}
      </div>

      {data.alerts && data.alerts.length > 0 && (
        <>
          <div className={styles.fullwidthDivider}></div>
          <div className={styles.weatherAlerts}>
            {data.alerts.map((alert, index) => (
              <WeatherAlert
                key={index}
                alert={alert}
                language={detectedLanguage}
                translations={translations}
              />
            ))}
          </div>
        </>
      )}

      {!disableLLM && showLLMSnippet && (
        <div id="llm-snippet" ref={LLMSnippetRef}>
          {/* LLM Snippet would be implemented here */}
          <div>AI Weather Summary (placeholder)</div>
        </div>
      )}

      <MockRichFooter optionSet="weather">
        {provider && (
          <span>
            {translate(translations, 'Data from')}{' '}
            <a target="_blank" rel="noopener" href={provider.url}>
              {provider.name}
            </a>
          </span>
        )}
      </MockRichFooter>
    </div>
  );
};

export default Weather;
