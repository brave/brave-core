/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Only worth calling out when the local timezone genuinely is UTC (offset
// 0); otherwise a "GMT-5"-style suffix is redundant with what dateStyle/
// timeStyle already conveys via the reader's own system clock.
function utcSuffix(date: Date) {
  return date.getTimezoneOffset() === 0 ? ' UTC' : ''
}

// Matches the "Wednesday, November 18, 1970 at 12:34:56 PM" style used for
// backend-formatted timestamps (e.g. "Catalog last updated") so dates read
// consistently across tabs, rather than a terse locale-dependent numeric
// format like "19/08/2026, 15:12:17".
export function formatUnixEpochToLocalTime(epoch: number) {
  const date = new Date(epoch * 1000) // Convert seconds to milliseconds.
  const formatted = date.toLocaleString(undefined, {
    dateStyle: 'full',
    timeStyle: 'medium',
  })
  return `${formatted}${utcSuffix(date)}`
}

export function formatUnixEpochToLocalDate(epoch: number) {
  const date = new Date(epoch * 1000) // Convert seconds to milliseconds.
  return date.toLocaleDateString(undefined, { dateStyle: 'full' })
}

export function formatUnixEpochToLocalTimeOnly(epoch: number) {
  const date = new Date(epoch * 1000) // Convert seconds to milliseconds.
  const formatted = date.toLocaleTimeString(undefined, { timeStyle: 'medium' })
  return `${formatted}${utcSuffix(date)}`
}

// Windows epoch (used by `base::Time::ToDeltaSinceWindowsEpoch`) is
// microseconds since 1601-01-01, `WINDOWS_TO_UNIX_EPOCH_MS` below is the
// offset to the Unix epoch (1970-01-01) in milliseconds.
const WINDOWS_TO_UNIX_EPOCH_MS = BigInt(11644473600000)

// A raw epoch value's digit count distinguishes Unix seconds (~10 digits
// until year 2286) from Windows epoch microseconds (~17 digits); condition
// matcher timestamps may be stored in either format (see
// `condition_matcher_util.h`'s epoch operator matcher).
const WINDOWS_EPOCH_MIN_DIGITS = 15

// Below this, a plain number is more likely a count/threshold (e.g. a
// numerical condition's operand) than a real Unix timestamp; Unix seconds
// for any date from 2001-09-09 onward are at least this many digits.
const UNIX_EPOCH_MIN_DIGITS = 9

// Returns a human-readable date/time if `value` is a plausible raw Unix or
// Windows-epoch timestamp, or `undefined` otherwise.
export function maybeFormatTimestamp(value: string): string | undefined {
  if (!/^\d+$/.test(value)) {
    return undefined
  }

  let epochMs: number
  if (value.length >= WINDOWS_EPOCH_MIN_DIGITS) {
    epochMs = Number(BigInt(value) / BigInt(1000) - WINDOWS_TO_UNIX_EPOCH_MS)
  } else if (value.length >= UNIX_EPOCH_MIN_DIGITS) {
    epochMs = Number(value) * 1000
  } else {
    return undefined
  }

  const date = new Date(epochMs)
  if (Number.isNaN(date.getTime())) {
    return undefined
  }

  const formatted = date.toLocaleString(undefined, {
    dateStyle: 'full',
    timeStyle: 'medium',
  })
  return `${formatted}${utcSuffix(date)}`
}

// 3 months or more out either way is rarely a meaningfully precise count
// (e.g. a campaign end date far in the future); naming the vague distance
// reads better than "in 1611 days".
const DISTANT_THRESHOLD_SECONDS = 90 * 86400

// A short "in 3 hours" / "5 days ago" companion to the full formatted date,
// so a future/past timestamp doesn't require doing the subtraction from "now"
// in your head. Only the single largest applicable unit is shown. Spelling
// out "1889 days, 8 hours, 1 minute, 47 seconds ago" is precision nobody
// asked for.
export function formatRelativeDuration(epoch: number) {
  const diffMs = epoch * 1000 - Date.now()
  const isPast = diffMs < 0
  const totalSeconds = Math.round(Math.abs(diffMs) / 1000)

  if (totalSeconds >= DISTANT_THRESHOLD_SECONDS) {
    return isPast ? 'in the distant past' : 'in the distant future'
  }

  const UNITS: Array<[number, string]> = [
    [86400, 'day'],
    [3600, 'hour'],
    [60, 'minute'],
    [1, 'second'],
  ]
  const [unitSeconds, unitName] =
    UNITS.find(([seconds]) => totalSeconds >= seconds) ?? UNITS.at(-1)!
  const count = Math.floor(totalSeconds / unitSeconds)

  const duration = `${count} ${unitName}${count === 1 ? '' : 's'}`
  return isPast ? `${duration} ago` : `in ${duration}`
}

// Splits off a trailing "(...)" from backend-formatted strings that embed a
// relative duration alongside an absolute date (e.g. "Friday, 21 August 2026
// at 12:42:05 (7 minutes ago)"), so the caller can grey out just that part,
// consistent with the equivalent JSX-built version of the same pattern
// elsewhere (e.g. campaigns.tsx's Start/End At).
export function splitTrailingParenthetical(value: string) {
  const match = /^(.*) (\([^)]*\))$/.exec(value)
  return match
    ? { main: match[1], parenthetical: match[2] }
    : { main: value, parenthetical: null }
}

export function uniqueTableRows<T>(data: T[]): T[] {
  return Array.from(new Set(data.map((item) => JSON.stringify(item))))
    .map((item) => JSON.parse(item))
}
