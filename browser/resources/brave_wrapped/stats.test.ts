// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  getDaysSinceFirstRun,
  getHdVideoHoursEquivalent,
  getMoviesEquivalent,
  getTimeSavedMs,
  hasEnoughData,
} from './stats'

const DAY_MS = 24 * 60 * 60 * 1000
const GB = 1024 * 1024 * 1024

function makeStats(
  adsBlocked: number,
  bandwidthSavedBytes = 0,
  firstRunTime = 0,
) {
  return { adsBlocked, bandwidthSavedBytes, firstRunTime }
}

describe('brave wrapped stats', () => {
  it('estimates 50ms saved per blocked item, like the NTP widget', () => {
    expect(getTimeSavedMs(makeStats(1000))).toBe(50_000)
  })

  it('counts whole two-hour movies', () => {
    expect(getMoviesEquivalent(makeStats(143_999))).toBe(0)
    expect(getMoviesEquivalent(makeStats(144_000))).toBe(1)
    expect(getMoviesEquivalent(makeStats(1_284_213))).toBe(8)
  })

  it('counts whole hours of HD video at 3 GB per hour', () => {
    expect(getHdVideoHoursEquivalent(makeStats(0, 3 * GB - 1))).toBe(0)
    expect(getHdVideoHoursEquivalent(makeStats(0, 9 * GB))).toBe(3)
  })

  it('returns 0 days when the install date is unknown', () => {
    expect(getDaysSinceFirstRun(makeStats(0))).toBe(0)
  })

  it('counts whole days since first run', () => {
    const stats = makeStats(0, 0, Date.now() - 731 * DAY_MS - 1000)
    expect(getDaysSinceFirstRun(stats)).toBe(731)
  })

  it('never returns negative days for a future first run', () => {
    expect(getDaysSinceFirstRun(makeStats(0, 0, Date.now() + DAY_MS))).toBe(0)
  })

  it('needs at least 100 blocked items to show the slides', () => {
    expect(hasEnoughData(makeStats(99))).toBe(false)
    expect(hasEnoughData(makeStats(100))).toBe(true)
  })
})
