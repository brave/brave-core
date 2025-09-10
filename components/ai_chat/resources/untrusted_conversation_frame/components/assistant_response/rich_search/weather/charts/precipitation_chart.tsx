import * as React from 'react';
import { Point, ThreeHourPrediction } from '../types';
import { createBezierPath, map } from './chart_utils';
import { translate, defaultTranslations } from '../stubs';
import styles from './precipitation_chart.module.scss';

interface PrecipitationChartProps {
  predictions: ThreeHourPrediction[];
  chosenDay?: number;
  dti18n: string;
  language?: string;
  translations?: typeof defaultTranslations;
}

const PrecipitationChart: React.FC<PrecipitationChartProps> = ({
  predictions,
  chosenDay = 0,
  dti18n,
  language = 'en-US',
  translations = defaultTranslations,
}: PrecipitationChartProps) => {
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
  const paddingTop = 30;
  const paddingBottom = 30;
  const { width, height } = dimensions;
  const effectiveHeight = height - paddingTop - paddingBottom;

  // Filter predictions with precipitation data
  const filtered = predictions.filter((p) => p.rain !== null);

  if (filtered.length === 0) {
    return (
      <div ref={containerRef} className={styles.precipitationChart}>
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
  const ys = filtered.map((p) => (p.rain as number) * 100);

  const xmin = 0;
  const xmax = Math.max(...xs);
  const ymin = 0;
  const ymax = 100;

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
  const gradientPath = `${createBezierPath(points)} L ${points[points.length - 1]?.[0]},${effectiveHeight + paddingTop} L ${points[0]?.[0]},${effectiveHeight + paddingTop} Z`;

  return (
    <div ref={containerRef} className={styles.precipitationChart}>
      <svg viewBox={`0 0 ${width} ${height}`}>
        {path && (
          <>
            <defs>
              <linearGradient id="precipitation-gradient" x1="0" y1="0" x2="0" y2="1">
                <stop stopColor="var(--precipitation-gradient-0-0)" offset="0" />
                <stop stopColor="var(--precipitation-gradient-0-5)" offset="0.5" />
                <stop stopColor="var(--precipitation-gradient-1-0)" offset="1" />
              </linearGradient>
            </defs>
            {/* gradient path below precipitation line */}
            <path d={gradientPath} fill="url(#precipitation-gradient)" />
            {/* precipitation line */}
            <path d={path} stroke="var(--line-color)" strokeWidth="2" fill="none" />
            {/* precipitation labels above line */}
            {points.map(([x, y], index) => {
              const rain = filtered[index]?.rain ?? 0;
              return (
                <text
                  key={index}
                  className="desktop-xsmall-regular t-tertiary"
                  x={x}
                  y={y - 12}
                  textAnchor={
                    index === 0 ? 'start' : index >= points.length - 1 ? 'end' : 'middle'
                  }
                >
                  {Math.round(rain * 100)}%
                </text>
              );
            })}
            {/* hour labels below line*/}
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

export default PrecipitationChart;
