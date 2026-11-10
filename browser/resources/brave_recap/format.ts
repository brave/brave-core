// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Matches the NTP stats widget so users see the same numbers in both places.

export interface FormattedValue {
  value: string
  unit: string
}

function splitParts(parts: Intl.NumberFormatPart[]): FormattedValue {
  let value = ''
  let unit = ''
  for (const part of parts) {
    if (part.type === 'unit') {
      unit += part.value
    } else if (part.type !== 'literal') {
      value += part.value
    }
  }
  return { value, unit }
}

const numberFormats = new Map<string, Intl.NumberFormat>()

// Formatters are costly to create and the count-up formats every frame.
function getNumberFormat(options: Intl.NumberFormatOptions = {}) {
  const key = JSON.stringify(options)
  let format = numberFormats.get(key)
  if (!format) {
    format = new Intl.NumberFormat(undefined, options)
    numberFormats.set(key, format)
  }
  return format
}

function formatUnit(
  value: number,
  unit: string,
  unitDisplay: 'long' | 'short',
  maximumFractionDigits = 0,
) {
  return splitParts(
    getNumberFormat({
      style: 'unit',
      unit,
      unitDisplay,
      maximumFractionDigits,
      roundingMode: 'ceil',
    } as Intl.NumberFormatOptions).formatToParts(value),
  )
}

export function formatCount(value: number) {
  return getNumberFormat().format(Math.round(value))
}

const SECOND_MS = 1000
const MINUTE_MS = 60 * SECOND_MS
const HOUR_MS = 60 * MINUTE_MS
const DAY_MS = 24 * HOUR_MS

// The unit is picked from `unitFor`, so a counting-up value can keep the unit
// of its final value.
export function formatTimeSaved(ms: number, unitFor = ms): FormattedValue {
  if (unitFor >= DAY_MS) {
    return formatUnit(ms / DAY_MS, 'day', 'long', 2)
  }
  if (unitFor >= HOUR_MS) {
    return formatUnit(ms / HOUR_MS, 'hour', 'long', 1)
  }
  if (unitFor >= MINUTE_MS) {
    return formatUnit(ms / MINUTE_MS, 'minute', 'long')
  }
  return formatUnit(Math.max(ms, 0) / SECOND_MS, 'second', 'long')
}

const KB = 1024
const MB = 1024 * KB
const GB = 1024 * MB

export function formatBandwidth(
  bytes: number,
  unitFor = bytes,
): FormattedValue {
  if (unitFor >= GB) {
    return formatUnit(bytes / GB, 'gigabyte', 'short', 2)
  }
  if (unitFor >= MB) {
    return formatUnit(bytes / MB, 'megabyte', 'short', 1)
  }
  return formatUnit(bytes / KB, 'kilobyte', 'short')
}

export function joinValue({ value, unit }: FormattedValue) {
  return `${value} ${unit}`
}

export function formatMonthYear(ms: number) {
  return new Intl.DateTimeFormat(undefined, {
    month: 'long',
    year: 'numeric',
  }).format(new Date(ms))
}
