/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StateStore, createStateStore } from '$web-common/state_store'

// Keys match the literal column labels produced by `BuildAdsInternals` in
// //brave/components/brave_ads/core/internal/ads_internals/ads_internals_util.cc
export interface ConversionUrlPattern {
  'URL Pattern': string
  'Expires At': number
}

export interface AdEvent {
  'Target URL': string
  'Ad Type': string
  'Event Type': string
  'Created At': number
}

export interface DiagnosticEntry {
  name: string
  value: string
}

export interface AppState {
  rewardsEnabled: boolean
  conversionUrlPatterns: ConversionUrlPattern[]
  adEvents: AdEvent[]
  diagnosticId: string
  diagnosticEntries: DiagnosticEntry[]
  actions: {
    loadAdsInternals: () => void
    clearAdsData: () => Promise<boolean>
    setDiagnosticId: (diagnosticId: string) => void
  }
}

export type AppStore = StateStore<AppState>

export function defaultAppStore() {
  return createStateStore<AppState>({
    rewardsEnabled: false,
    conversionUrlPatterns: [],
    adEvents: [],
    diagnosticId: '',
    diagnosticEntries: [],
    actions: {
      loadAdsInternals() {},
      async clearAdsData() {
        return false
      },
      setDiagnosticId(diagnosticId) {},
    },
  })
}
