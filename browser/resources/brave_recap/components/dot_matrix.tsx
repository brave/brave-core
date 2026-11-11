// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Distance between dot centers, in CSS pixels.
const PITCH = 12
const MIN_RADIUS = 0.35
const MAX_RADIUS = 1.8
// Extra radius the shimmer adds at its brightest.
const SHIMMER_GROW = 0.2
const SHIMMER_OPACITY = 0.5
// The shimmer's travel plus rest, in ms.
const SHIMMER_CYCLE_MS = 9000
// The part of the cycle spent sweeping; the rest is a pause.
const SHIMMER_SWEEP = 0.55
// Half the shimmer's width, as a fraction of the canvas width.
const SHIMMER_HALF_WIDTH = 0.35
// How far the band leans.
const SHIMMER_SLANT = 0.17
// Frame rate cap; the motion is slow so there's no need for more.
const FRAME_MS = 1000 / 30
// Orange is drawn in this many opacity steps so dots can share a path.
const SHIMMER_STEPS = 12
// The one frame drawn when motion is reduced, picked for a varied arrangement.
const STATIC_TIME_MS = 4000

const FALLBACK_DOT_COLOR = '#000'
const FALLBACK_HIGHLIGHT_COLOR = '#ff4000'

function smoothstep(edge0: number, edge1: number, x: number) {
  const t = Math.min(1, Math.max(0, (x - edge0) / (edge1 - edge0)))
  return t * t * (3 - 2 * t)
}

// A slowly drifting field in [0, 1] that sets how big each dot is, so the
// halftone clusters swell and shrink and move across the page.
function dotSize(x: number, y: number, seconds: number) {
  const n =
    Math.sin(x * 0.011 + seconds * 0.35) * Math.sin(y * 0.015 - seconds * 0.27)
    + Math.sin((x + y) * 0.0055 + seconds * 0.22)
    + Math.sin(x * 0.004 - y * 0.006 + seconds * 0.15)
  return smoothstep(-1.1, 1.6, n)
}

// Where the shimmer's center is along the slanted axis, or null while it is
// resting off-screen.
function shimmerCenter(ms: number, width: number, height: number) {
  const phase = (ms % SHIMMER_CYCLE_MS) / SHIMMER_CYCLE_MS
  if (phase >= SHIMMER_SWEEP) {
    return null
  }
  const progress = phase / SHIMMER_SWEEP
  const eased = 0.5 - 0.5 * Math.cos(Math.PI * progress)
  const half = width * SHIMMER_HALF_WIDTH
  const length = width + height * SHIMMER_SLANT
  return -half + eased * (length + 2 * half)
}

function readColor(element: Element, name: string, fallback: string) {
  return getComputedStyle(element).getPropertyValue(name).trim() || fallback
}

function draw(
  ctx: CanvasRenderingContext2D,
  width: number,
  height: number,
  ms: number,
  dotColor: string,
  highlightColor: string,
) {
  ctx.clearRect(0, 0, width, height)

  const seconds = ms / 1000
  const center = shimmerCenter(ms, width, height)
  const half = width * SHIMMER_HALF_WIDTH
  const columns = Math.ceil(width / PITCH) + 1
  const rows = Math.ceil(height / PITCH) + 1

  const orange: Array<Array<number>> = Array.from(
    { length: SHIMMER_STEPS },
    () => [],
  )

  ctx.fillStyle = dotColor
  ctx.beginPath()
  for (let row = 0; row < rows; row++) {
    const y = row * PITCH
    for (let column = 0; column < columns; column++) {
      const x = column * PITCH
      const radius =
        MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS) * dotSize(x, y, seconds)
      ctx.moveTo(x + radius, y)
      ctx.arc(x, y, radius, 0, Math.PI * 2)

      if (center === null) {
        continue
      }
      const k = 1 - Math.abs(x + y * SHIMMER_SLANT - center) / half
      if (k <= 0) {
        continue
      }
      const intensity = k * k * (3 - 2 * k)
      const step = Math.min(
        SHIMMER_STEPS - 1,
        Math.floor(intensity * SHIMMER_STEPS),
      )
      orange[step].push(x, y, radius + SHIMMER_GROW * intensity)
    }
  }
  ctx.fill()

  ctx.fillStyle = highlightColor
  for (let step = 0; step < SHIMMER_STEPS; step++) {
    const dots = orange[step]
    if (dots.length === 0) {
      continue
    }
    ctx.globalAlpha = ((step + 1) / SHIMMER_STEPS) * SHIMMER_OPACITY
    ctx.beginPath()
    for (let i = 0; i < dots.length; i += 3) {
      ctx.moveTo(dots[i] + dots[i + 2], dots[i + 1])
      ctx.arc(dots[i], dots[i + 1], dots[i + 2], 0, Math.PI * 2)
    }
    ctx.fill()
  }
  ctx.globalAlpha = 1
}

// The page background: a grid of dots whose sizes drift, with an orange
// highlight that sweeps across and tints the dots it passes. Drawn per dot
// rather than as an image so the highlight has no banding.
export function DotMatrix() {
  const canvasRef = React.useRef<HTMLCanvasElement>(null)

  React.useEffect(() => {
    const canvas = canvasRef.current
    const ctx = canvas?.getContext('2d')
    if (!canvas || !ctx) {
      return
    }

    const motion = window.matchMedia?.('(prefers-reduced-motion: reduce)')
    const dotColor = readColor(canvas, '--recap-dot-color', FALLBACK_DOT_COLOR)
    const highlightColor = readColor(
      canvas,
      '--recap-highlight',
      FALLBACK_HIGHLIGHT_COLOR,
    )

    let width = 0
    let height = 0
    let frame = 0
    let last = -Infinity
    let start: number | undefined

    const render = (ms: number) => {
      draw(ctx, width, height, ms, dotColor, highlightColor)
    }

    const tick = (now: number) => {
      frame = requestAnimationFrame(tick)
      if (now - last < FRAME_MS) {
        return
      }
      last = now
      start ??= now
      render(now - start)
    }

    const play = () => {
      cancelAnimationFrame(frame)
      if (motion?.matches) {
        render(STATIC_TIME_MS)
      } else {
        frame = requestAnimationFrame(tick)
      }
    }

    const resize = () => {
      const ratio = window.devicePixelRatio || 1
      width = canvas.clientWidth
      height = canvas.clientHeight
      canvas.width = Math.round(width * ratio)
      canvas.height = Math.round(height * ratio)
      ctx.setTransform(ratio, 0, 0, ratio, 0, 0)
      play()
    }

    resize()
    const observer =
      typeof ResizeObserver === 'undefined'
        ? undefined
        : new ResizeObserver(resize)
    observer?.observe(canvas)
    motion?.addEventListener('change', play)

    return () => {
      cancelAnimationFrame(frame)
      observer?.disconnect()
      motion?.removeEventListener('change', play)
    }
  }, [])

  return (
    <canvas
      className='dot-matrix'
      ref={canvasRef}
      aria-hidden='true'
    />
  )
}
