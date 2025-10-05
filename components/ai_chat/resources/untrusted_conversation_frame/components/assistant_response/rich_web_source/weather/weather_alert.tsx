import * as React from 'react';
import { WeatherAlert as WeatherAlertType } from './types';
import { InfoOutlineIcon, ClockIcon, MockInterpunctSpacer } from './stubs';
import { translate, defaultTranslations } from './stubs';
import styles from './weather_alert.module.scss';

interface WeatherAlertProps {
  alert: WeatherAlertType;
  language?: string;
  translations?: typeof defaultTranslations;
}

// Custom hook to detect overflow
const useDetectOverflow = (options: { x?: boolean; y?: boolean } = {}) => {
  const [overflowing, setOverflowing] = React.useState(false);
  const elementRef = React.useRef<HTMLDivElement>(null);

  React.useEffect(() => {
    const checkOverflow = () => {
      const element = elementRef.current;
      if (!element) return;

      const isOverflowing =
        (options.x !== false && element.scrollWidth > element.clientWidth) ||
        (options.y !== false && element.scrollHeight > element.clientHeight);

      setOverflowing(isOverflowing);
    };

    const resizeObserver = new ResizeObserver(checkOverflow);
    const element = elementRef.current;

    if (element) {
      resizeObserver.observe(element);
      checkOverflow();
    }

    return () => {
      if (element) {
        resizeObserver.unobserve(element);
      }
    };
  }, [options.x, options.y]);

  return { overflowing, elementRef };
};

const WeatherAlert: React.FC<WeatherAlertProps> = ({
  alert,
  language = 'en-US',
  translations = defaultTranslations,
}) => {
  const [expanded, setExpanded] = React.useState(false);
  const { overflowing, elementRef } = useDetectOverflow({ x: false });

  const toggleExpanded = () => {
    setExpanded(!expanded);
  };

  return (
    <div className={styles.alert}>
      <div className={`${styles.header} desktop-small-semibold`}>
        <InfoOutlineIcon />
        {translate(translations, 'Extreme weather advisory')}
        <div className={styles.spacer}>•</div>
        {alert.start_relative_i18n && (
          <>
            <ClockIcon />
            <span>{alert.start_relative_i18n}</span>
          </>
        )}
      </div>
      <div className={`${styles.event} desktop-small-semibold t-secondary`}>
        <span>{alert.event}</span>
        {alert.sender && (
          <>
            <MockInterpunctSpacer />
            <span className="desktop-small-regular text-ellipsis">{alert.sender}</span>
          </>
        )}
      </div>
      <div
        ref={elementRef}
        className={`desktop-small-regular t-secondary ${!expanded ? styles.lineClamp2 : ''}`}
      >
        {alert.description}
      </div>
      {(overflowing || expanded) && (
        <button
          className={`${styles.showMore} desktop-small-semibold t-link`}
          onClick={toggleExpanded}
        >
          {expanded
            ? translate(translations, 'Show less')
            : translate(translations, 'Show more')}
        </button>
      )}
    </div>
  );
};

export default WeatherAlert;
