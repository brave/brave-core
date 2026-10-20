/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const byteUnits = ['byte', 'kilobyte', 'megabyte', 'gigabyte', 'terabyte']

export function formatBytes(size: bigint | number) {
  let value = Number(size)
  let unit = 0
  while (value >= 1000 && unit < byteUnits.length - 1) {
    value /= 1000
    unit++
  }
  return new Intl.NumberFormat(undefined, {
    style: 'unit',
    unit: byteUnits[unit],
    unitDisplay: unit === 0 ? 'long' : 'short',
    maximumFractionDigits: unit === 0 || value >= 100 ? 0 : 1,
  }).format(value)
}

function isSameDay(a: Date, b: Date) {
  return (
    a.getFullYear() === b.getFullYear()
    && a.getMonth() === b.getMonth()
    && a.getDate() === b.getDate()
  )
}

// Formats a time as just the time of day if it is today, or as a short date
// and time otherwise.
export function formatTime(time: number, now = Date.now()) {
  const date = new Date(time)
  if (isSameDay(date, new Date(now))) {
    return date.toLocaleTimeString(undefined, { timeStyle: 'short' })
  }
  return date.toLocaleString(undefined, {
    dateStyle: 'medium',
    timeStyle: 'short',
  })
}

export function isLink(text: string) {
  return /^https?:\/\/\S+$/i.test(text.trim())
}
