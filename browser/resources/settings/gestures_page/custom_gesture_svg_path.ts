/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {resampleToCount, normalizeScaleAndTranslation} from './custom_gesture_geometry.js'
import type {Point} from './custom_gesture_geometry.js'

const LIVE_DRAW_RESAMPLE_POINT_COUNT = 24

// Kept separate from `normalizeScaleAndTranslation`, which the similarity
// metric also uses; centering there would add per-draw noise to it.
function centerInUnitSquare(path: Point[]): Point[] {
  const maxX = Math.max(...path.map((point) => point.x))
  const maxY = Math.max(...path.map((point) => point.y))
  const offsetX = (1 - maxX) / 2
  const offsetY = (1 - maxY) / 2
  return path.map((point) => ({
    x: offsetX + point.x,
    y: offsetY + point.y,
  }))
}

// Approximates a Catmull-Rom spline with cubic-bezier control points.
function catmullRomControlPoints(
  previousPoint: Point, segmentStart: Point, segmentEnd: Point,
  nextPoint: Point): { controlPoint1: Point, controlPoint2: Point } {
  // Standard tangent scale for approximating a uniform Catmull-Rom spline
  // with cubic-bezier control points.
  const CATMULL_ROM_TANGENT_SCALE = 6
  const controlPoint1 = {
    x: segmentStart.x +
      (segmentEnd.x - previousPoint.x) / CATMULL_ROM_TANGENT_SCALE,
    y: segmentStart.y +
      (segmentEnd.y - previousPoint.y) / CATMULL_ROM_TANGENT_SCALE,
  }
  const controlPoint2 = {
    x: segmentEnd.x -
      (nextPoint.x - segmentStart.x) / CATMULL_ROM_TANGENT_SCALE,
    y: segmentEnd.y -
      (nextPoint.y - segmentStart.y) / CATMULL_ROM_TANGENT_SCALE,
  }
  return { controlPoint1, controlPoint2 }
}

// Smooths the raw captured points, since they would otherwise render as a
// jagged polyline with sharp corners.
export function svgPathData(path: Point[]): string {
  if (path.length < 2) {
    return ''
  }
  if (path.length === 2) {
    return `M ${path[0].x} ${path[0].y} L ${path[1].x} ${path[1].y}`
  }

  let pathData = `M ${path[0].x} ${path[0].y}`
  for (let i = 0; i < path.length - 1; ++i) {
    const previousPoint = path[Math.max(i - 1, 0)]
    const segmentStart = path[i]
    const segmentEnd = path[i + 1]
    const nextPoint = path[Math.min(i + 2, path.length - 1)]
    const { controlPoint1, controlPoint2 } = catmullRomControlPoints(
      previousPoint, segmentStart, segmentEnd, nextPoint)
    pathData += ` C ${controlPoint1.x} ${controlPoint1.y}, ` +
      `${controlPoint2.x} ${controlPoint2.y}, ${segmentEnd.x} ${segmentEnd.y}`
  }
  return pathData
}

// Preserves aspect ratio, so a tall or wide shape does not get stretched
// to fill the square preview.
export function fitToViewBox(
    path: Point[], size: number, padding: number): Point[] {
  // Resample before normalizing, so the preview's curve does not inherit
  // any jitter in the originally-recorded raw points.
  const resampledPoints = resampleToCount(path, LIVE_DRAW_RESAMPLE_POINT_COUNT)
  const normalizedPoints = normalizeScaleAndTranslation(resampledPoints)
  const centeredPoints = centerInUnitSquare(normalizedPoints)
  const innerSize = size - padding * 2
  return centeredPoints.map((point) => ({
    x: padding + point.x * innerSize,
    y: padding + point.y * innerSize,
  }))
}
