// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Time and bandwidth formatting matches the NTP stats widget
// (brave_new_tab_page_refresh/components/widgets/stats_widget.tsx) so the
// numbers here are identical to what users already see on the NTP.

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

function formatUnit(
  value: number,
  unit: string,
  unitDisplay: 'long' | 'short',
  maximumFractionDigits = 0,
) {
  return splitParts(
    new Intl.NumberFormat(undefined, {
      style: 'unit',
      unit,
      unitDisplay,
      maximumFractionDigits,
      roundingMode: 'ceil',
    } as Intl.NumberFormatOptions).formatToParts(value),
  )
}

export function formatCount(value: number) {
  return new Intl.NumberFormat().format(Math.round(value))
}

export function formatTimeSaved(ms: number): FormattedValue {
  const seconds = ms / 1000
  const minutes = seconds / 60
  const hours = minutes / 60
  const days = hours / 24
  if (days >= 1) {
    return formatUnit(days, 'day', 'long', 2)
  }
  if (hours >= 1) {
    return formatUnit(hours, 'hour', 'long', 1)
  }
  if (minutes >= 1) {
    return formatUnit(minutes, 'minute', 'long')
  }
  return formatUnit(Math.max(seconds, 0), 'second', 'long')
}

export function formatBandwidth(bytes: number): FormattedValue {
  const kb = bytes / 1024
  const mb = kb / 1024
  const gb = mb / 1024
  if (gb >= 1) {
    return formatUnit(gb, 'gigabyte', 'short', 2)
  }
  if (mb >= 1) {
    return formatUnit(mb, 'megabyte', 'short', 1)
  }
  return formatUnit(kb, 'kilobyte', 'short')
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
