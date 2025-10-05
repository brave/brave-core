import * as React from 'react';
import { Theme, ThreeHourPrediction } from '../types';
import WindSvgs from './wind_svgs';
import { convertWindSpeed } from './chart_utils';
import { getWindDirection, getBeaufortScale } from '../utils';
import { translate, defaultTranslations } from '../stubs';
import styles from './wind_graphic.module.scss';

interface WindGraphicProps {
  predictions: ThreeHourPrediction[];
  units: 'us' | 'metric';
  speedLabel: string;
  theme: Theme.light | Theme.dark;
  chosenDay?: number;
  dti18n: string;
  language?: string;
  translations?: typeof defaultTranslations;
}

const WindGraphic: React.FC<WindGraphicProps> = ({
  predictions,
  units,
  speedLabel,
  theme,
  chosenDay = 0,
  dti18n,
  language = 'en-US',
  translations = defaultTranslations,
}: WindGraphicProps) => {
  const mapped = React.useMemo(() =>
    predictions.map((p) => {
      if (!p.wind) {
        return p;
      }
      return {
        ...p,
        wind: {
          ...p.wind,
          speed: `${Math.round(convertWindSpeed(p.wind.speed, units))} ${speedLabel}`,
          deg: getWindDirection(p.wind.deg),
          beaufortScale: getBeaufortScale(p.wind.speed),
        },
      };
    }), [predictions, units, speedLabel]);

  if (mapped.length === 0) {
    return (
      <div className={styles.windGraphic}>
        <div className={styles.message}>
          {translate(translations, 'detailed-weather-data-not-available-for-{date}', {
            date: dti18n,
          })}
        </div>
      </div>
    );
  }

  return (
    <div className={styles.windGraphic}>
      <div className={styles.items}>
        {mapped.map((pred, index) => (
          <div
            key={index}
            className={`${styles.item} ${
              index === 0
                ? 'desktop-xsmall-semibold t-primary'
                : 'desktop-xsmall-regular t-tertiary'
            } ${pred.wind === null ? styles.invisible : ''}`}
          >
            {pred.wind !== null && (
              <>
                <div className={styles.speed}>{pred.wind.speed}</div>
                <div className={styles.direction}>{pred.wind.deg}</div>
                {pred.wind.beaufortScale && (
                  <div className={styles.icon}>
                    <WindSvgs beaufortScale={pred.wind.beaufortScale} theme={theme} />
                  </div>
                )}
              </>
            )}
          </div>
        ))}
      </div>
      <div className={styles.times}>
        {mapped.map((pred, index) => (
          <div key={index} className={`${styles.time} desktop-xsmall-regular t-tertiary`}>
            {pred.wind !== null && (
              <>
                {chosenDay === 0 && index === 0
                  ? translate(translations, 'Now')
                  : pred?.hour}
              </>
            )}
          </div>
        ))}
      </div>
    </div>
  );
};

export default WindGraphic;
