// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export interface WrappedStats {
  adsBlocked: number
  bandwidthSavedBytes: number
  // Milliseconds since the Unix epoch, or 0 if unknown.
  firstRunTime: number
}

// Same estimate the NTP stats widget uses.
const MS_SAVED_PER_BLOCKED_ITEM = 50

const MS_PER_MOVIE = 2 * 60 * 60 * 1000
const BYTES_PER_HD_VIDEO_HOUR = 3 * 1024 * 1024 * 1024
const MS_PER_DAY = 24 * 60 * 60 * 1000

// Below this the page shows an empty state instead of the slides.
const MIN_BLOCKED_TO_SHOW = 100

export function getTimeSavedMs(stats: WrappedStats) {
  return stats.adsBlocked * MS_SAVED_PER_BLOCKED_ITEM
}

export function getMoviesEquivalent(stats: WrappedStats) {
  return Math.floor(getTimeSavedMs(stats) / MS_PER_MOVIE)
}

export function getHdVideoHoursEquivalent(stats: WrappedStats) {
  return Math.floor(stats.bandwidthSavedBytes / BYTES_PER_HD_VIDEO_HOUR)
}

export function getDaysSinceFirstRun(stats: WrappedStats) {
  if (!stats.firstRunTime) {
    return 0
  }
  return Math.max(0, Math.floor((Date.now() - stats.firstRunTime) / MS_PER_DAY))
}

export function hasEnoughData(stats: WrappedStats) {
  return stats.adsBlocked >= MIN_BLOCKED_TO_SHOW
}
