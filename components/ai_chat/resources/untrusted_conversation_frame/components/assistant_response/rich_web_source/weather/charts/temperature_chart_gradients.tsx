import * as React from 'react';
import { Point } from '../types';
import { createBezierPath } from './chart_utils';

interface TemperatureChartGradientsProps {
  points: Point[];
  freezingLevelY: number;
}

// Split points into chunks based on freezing level
function splitPoints(points: Point[], freezingLevelY: number) {
  let chunks: {
    above: Point[][];
    below: Point[][];
  } = {
    above: [],
    below: [],
  };
  let above: Point[] = [];
  let below: Point[] = [];

  points.forEach(([x, y], i) => {
    if (i === 0) {
      if (y <= freezingLevelY) {
        above.push([x, y]);
      } else {
        below.push([x, y]);
      }
    } else {
      const prev = points[i - 1];
      if (!prev) {
        return;
      }
      const prevY = prev[1];
      if (y <= freezingLevelY && prevY <= freezingLevelY) {
        above.push([x, y]);
      } else if (y > freezingLevelY && prevY > freezingLevelY) {
        below.push([x, y]);
      } else {
        const x1 = prev[0];
        const y1 = prev[1];
        const x2 = x;
        const y2 = y;
        const t = (freezingLevelY - y1) / (y2 - y1);
        const crossX = x1 + t * (x2 - x1);
        const crossY = freezingLevelY;

        if (y2 <= freezingLevelY) {
          above.push([crossX, crossY]);
          below.push([crossX, crossY]);
          chunks.below.push(below);
          below = [];
          above.push([x, y]);
        } else {
          below.push([crossX, crossY]);
          above.push([crossX, crossY]);
          chunks.above.push(above);
          above = [];
          below.push([x, y]);
        }
      }
    }
  });

  if (below.length) {
    chunks.below.push(below);
  }
  if (above.length) {
    chunks.above.push(above);
  }

  return chunks;
}

const createGradientPath = (chunks: Point[][], freezingLevelY: number) =>
  chunks
    .map((chunk) => {
      const first = chunk[0];
      const last = chunk[chunk.length - 1];
      if (!first || !last) {
        return '';
      }
      return `${createBezierPath(chunk)} L ${last[0]},${freezingLevelY} L ${first[0]},${freezingLevelY} Z`;
    })
    .join(' ');

const TemperatureChartGradients: React.FC<TemperatureChartGradientsProps> = ({
  points,
  freezingLevelY
}: TemperatureChartGradientsProps) => {
  const chunks = React.useMemo(() => splitPoints(points, freezingLevelY), [points, freezingLevelY]);

  const positiveGradientPath = React.useMemo(
    () => createGradientPath(chunks.above, freezingLevelY),
    [chunks.above, freezingLevelY]
  );

  const negativeGradientPath = React.useMemo(
    () => createGradientPath(chunks.below, freezingLevelY),
    [chunks.below, freezingLevelY]
  );

  return (
    <>
      <defs>
        <linearGradient id="temperature-gradient-pos" x1="0" y1="0" x2="0" y2="1">
          <stop stopColor="var(--temperature-gradient-pos-0-0)" offset="0" />
          <stop stopColor="var(--temperature-gradient-pos-0-5)" offset="0.5" />
          <stop stopColor="var(--temperature-gradient-pos-1-0)" offset="1" />
        </linearGradient>
        <linearGradient id="temperature-gradient-neg" x1="0" y1="0" x2="0" y2="1">
          <stop stopColor="var(--temperature-gradient-neg-1-0)" offset="0" />
          <stop stopColor="var(--temperature-gradient-neg-0-5)" offset="0.5" />
          <stop stopColor="var(--temperature-gradient-neg-0-0)" offset="1" />
        </linearGradient>
      </defs>
      <path d={positiveGradientPath} fill="url(#temperature-gradient-pos)" />
      <path d={negativeGradientPath} fill="url(#temperature-gradient-neg)" />
    </>
  );
};

export default TemperatureChartGradients;
