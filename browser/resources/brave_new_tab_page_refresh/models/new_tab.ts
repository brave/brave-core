/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export type ClockFormat = '' | 'h12' | 'h24'

export interface ShieldsStats {
  bandwidthSavedBytes: number
  adsBlocked: number
}

export interface NewTabState {
  showClock: boolean
  clockFormat: ClockFormat
  showShieldsStats: boolean
  shieldsStats: ShieldsStats | null
}

export function defaultNewTabState(): NewTabState {
  return {
    showClock: false,
    clockFormat: '',
    showShieldsStats: false,
    shieldsStats: null
  }
}

export interface NewTabActions {
  setShowClock: (showClock: boolean) => void
  setClockFormat: (format: ClockFormat) => void
  setShowShieldsStats: (showShieldsStats: boolean) => void
}

export function defaultNewTabActions(): NewTabActions {
  return {
    setShowClock(showClock) {},
    setClockFormat(format) {},
    setShowShieldsStats(showShieldsStats) {}
  }
}
