import * as React from 'react';
import { Point, ThreeHourPrediction } from '../types';
import { createBezierPath, map } from './chart_utils';
import TemperatureChartGradients from './temperature_chart_gradients';
import { translate, defaultTranslations } from '../stubs';
import styles from './temperature_chart.module.scss';

interface TemperatureChartProps {
  predictions: ThreeHourPrediction[];
  units: 'us' | 'metric';
  chosenDay?: number;
  dti18n: string;
  language?: string;
  translations?: typeof defaultTranslations;
}

const TemperatureChart: React.FC<TemperatureChartProps> = ({
  predictions,
  units,
  chosenDay = 0,
  dti18n,
  language = 'en-US',
  translations = defaultTranslations,
}: TemperatureChartProps) => {
  const containerRef = React.useRef<HTMLDivElement>(null);
  const [dimensions, setDimensions] = React.useState({ width: 364, height: 122 });

  // Update dimensions when container size changes
  React.useEffect(() => {
    if (!containerRef.current) return;

    const updateDimensions = () => {
      const { offsetWidth, offsetHeight } = containerRef.current!;
      if (offsetWidth > 0 && offsetHeight > 0) {
        setDimensions({ width: offsetWidth, height: offsetHeight });
      }
    };

    const resizeObserver = new ResizeObserver(updateDimensions);
    resizeObserver.observe(containerRef.current);
    updateDimensions();

    return () => resizeObserver.disconnect();
  }, []);

  // Constants
  const paddingTop = 20;
  const paddingBottom = 20;
  const { width, height } = dimensions;
  const effectiveHeight = height - paddingTop - paddingBottom;
  const freezingLevel = units === 'metric' ? 0 : 32;

  // Filter predictions with temperature data
  const filtered = predictions.filter((p) => p.temp[units] !== null);

  if (filtered.length === 0) {
    return (
      <div ref={containerRef} className={styles.temperatureChartContainer}>
        <svg viewBox={`0 0 ${width} ${height}`}>
          <text
            className="desktop-small-regular t-tertiary"
            y="50%"
            x="50%"
            textAnchor="middle"
          >
            {translate(translations, 'detailed-weather-data-not-available-for-{date}', {
              date: dti18n,
            })}
          </text>
        </svg>
      </div>
    );
  }

  // Generate x, y coordinates from the real world
  const xs = [...filtered.keys()];
  const ys = filtered.map((p) => Math.round(p.temp[units] as number));

  const xmin = 0;
  const xmax = Math.max(...xs);
  const ymin = Math.min(...ys, freezingLevel);
  const ymax = Math.max(...ys, freezingLevel);

  // Map x, y coordinates from the real world to the SVG world
  const points = xs
    .map((x, i) => [x, ys[i] as number] as const)
    .map(
      (point) =>
        [
          map(point[0], xmin, xmax, 0, width),
          map(point[1], ymin, ymax, effectiveHeight, 0) + paddingTop,
        ] as Point,
    );

  const path = createBezierPath(points);
  const freezingLevelY = map(freezingLevel, ymin, ymax, effectiveHeight, 0) + paddingTop;

  return (
    <div ref={containerRef} className={styles.temperatureChartContainer}>
      <svg viewBox={`0 0 ${width} ${height}`}>
        {path && (
          <>
            {/* gradient paths, above/below temperature line */}
            <TemperatureChartGradients points={points} freezingLevelY={freezingLevelY} />
            {/* temperature line */}
            <path d={path} stroke="var(--line-color)" strokeWidth="2" fill="none" />
            {/* temperature labels above line */}
            {points.map(([x, y], index) => (
              <text
                key={index}
                className="desktop-xsmall-regular t-tertiary"
                x={x}
                y={y - 8}
                textAnchor={
                  index === 0 ? 'start' : index >= points.length - 1 ? 'end' : 'middle'
                }
              >
                {Math.round(filtered[index]?.temp?.[units] ?? 0)}°
              </text>
            ))}
            {/* hour labels below line */}
            {points.map(([x], index) => (
              <text
                key={index}
                className="desktop-xsmall-regular t-tertiary"
                x={x}
                y={height - 5}
                textAnchor={
                  index === 0 ? 'start' : index >= points.length - 1 ? 'end' : 'middle'
                }
              >
                {chosenDay === 0 && index === 0
                  ? translate(translations, 'Now')
                  : filtered[index]?.hour}
              </text>
            ))}
          </>
        )}
      </svg>
    </div>
  );
};

export default TemperatureChart;
