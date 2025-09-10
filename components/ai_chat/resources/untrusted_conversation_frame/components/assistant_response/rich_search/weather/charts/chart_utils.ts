import { Point } from '../types';

export const map = (value: number, inMin: number, inMax: number, outMin: number, outMax: number) =>
  ((value - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;

export const line = (a: Point, b: Point) => {
  const lengthX = b[0] - a[0];
  const lengthY = b[1] - a[1];
  return {
    length: Math.sqrt(Math.pow(lengthX, 2) + Math.pow(lengthY, 2)),
    angle: Math.atan2(lengthY, lengthX),
  };
};

export const controlPoint = (
  current: Point,
  previous: Point | undefined,
  next: Point | undefined,
  reverse: boolean,
  smoothing: number = 0.15,
  flattening: number = 0.5,
) => {
  const p = previous || current;
  const n = next || current;
  const o = line(p, n);
  const flat = map(Math.cos(o.angle) * flattening, 0, 1, 1, 0);
  const angle = o.angle * flat + (reverse ? Math.PI : 0);
  const length = o.length * smoothing;
  const x = current[0] + Math.cos(angle) * length;
  const y = current[1] + Math.sin(angle) * length;
  return [x, y];
};

export const bezierCommand = (point: Point, index: number, array: Point[]) => {
  const prev = array[index - 1];
  if (!prev) {
    return;
  }
  const prev2 = array[index - 2];
  const next = array[index + 1];

  const cps = controlPoint(prev, prev2, point, false);
  const cpe = controlPoint(point, prev, next, true);
  return `C ${cps[0]},${cps[1]} ${cpe[0]},${cpe[1]} ${point[0]},${point[1]}`;
};

export const createBezierPath = (points: Point[]) => {
  return points.reduce(
    (acc, point, index, array) =>
      index === 0 ? `M ${point[0]},${point[1]}` : `${acc} ${bezierCommand(point, index, array)}`,
    '',
  );
};

const mpsToKph = (speed: number) => {
  return speed * 3.6;
};

const mpsToMph = (speed: number) => {
  return speed * 2.237;
};

export const convertWindSpeed = (speed: number, units: 'us' | 'metric') => {
  return units === 'metric' ? mpsToKph(speed) : mpsToMph(speed);
};