// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { formatBandwidth, formatTimeSaved, joinValue } from './format'

const SECOND_MS = 1000
const MINUTE_MS = 60 * SECOND_MS
const HOUR_MS = 60 * MINUTE_MS
const DAY_MS = 24 * HOUR_MS

const KB = 1024
const MB = 1024 * KB
const GB = 1024 * MB

// Units depend on the default locale, so mostly only numbers are checked.
describe('brave wrapped formatting', () => {
  it('picks the largest whole time unit', () => {
    expect(formatTimeSaved(30 * SECOND_MS).value).toBe('30')
    expect(formatTimeSaved(5 * MINUTE_MS).value).toBe('5')
    expect(formatTimeSaved(17.9 * HOUR_MS).value).toBe('17.9')
    expect(formatTimeSaved(2.5 * DAY_MS).value).toBe('2.5')
  })

  it('rounds time up so a non-zero saving never shows as less', () => {
    expect(formatTimeSaved(4.2 * MINUTE_MS).value).toBe('5')
  })

  it('never shows negative time', () => {
    expect(formatTimeSaved(-1).value).toBe('0')
  })

  it('picks the largest whole data unit', () => {
    expect(formatBandwidth(512 * KB).value).toBe('512')
    expect(formatBandwidth(1.5 * MB).value).toBe('1.5')
    expect(formatBandwidth(24.04 * GB).value).toBe('24.04')
  })

  it('returns a unit separate from the number', () => {
    const formatted = formatBandwidth(2 * GB)
    expect(formatted.unit).not.toBe('')
    expect(formatted.unit).not.toContain('2')
    expect(joinValue(formatted)).toBe(`2 ${formatted.unit}`)
  })
})
