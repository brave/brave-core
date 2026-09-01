/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StateStore, createStateStore } from '$web-common/state_store'

// Shared by the Events and Transactions tabs' `DateRangeFilter`. Lifted up
// here (rather than local component state) so the selection survives
// navigating away from the tab and back, the same way `autoRefreshEnabled`
// does. Deliberately not persisted to disk, so it resets on a fresh page
// load like the rest of this in-memory-only state.
export type DateRangePreset =
  'all' | 'today' | 'hour' | 'day' | 'week' | 'month' | 'custom'

export interface DateRangeFilterState {
  preset: DateRangePreset
  fromDate: string
  toDate: string
}

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

export interface ConfirmationQueueItem {
  'Transaction ID': string
  'Ad Type': string
  'Confirmation Type': string
  'Retry Count': number
  'Created At'?: number
  'Process At'?: number
  'User Data'?: Record<string, unknown>
}

export interface Transaction {
  'Transaction ID': string
  'Creative Instance ID': string
  'Ad Type': string
  'Confirmation Type': string
  'Value': number
  'Created At'?: number
  // Not yet redeemed for BAT if absent.
  'Reconciled At'?: number
}

export interface PaymentToken {
  'Transaction ID': string
  'Ad Type': string
  'Confirmation Type': string
  'Value'?: number
}

export interface CampaignCreative {
  'Creative Instance ID': string
  'Target URL': string
  // Only present for new tab page ads.
  'Company Name'?: string
  'Alt'?: string
  'Dynamic/Static'?: string
  // Only present for notification ads.
  'Title'?: string
  'Body'?: string
}

export interface CampaignCreativeSet {
  'Creative Set ID': string
  'Segments': string[]
  // Frequency caps for this creative set; 0 means unlimited. The "Served"
  // counterparts are how many served impressions currently count against
  // each cap's window.
  'Per Day': number
  'Per Day Served': number
  'Per Week': number
  'Per Week Served': number
  'Per Month': number
  'Per Month Served': number
  'Total Max': number
  'Total Max Served': number
  'Creatives': CampaignCreative[]
}

export interface CampaignDaypart {
  // A string of digits, Sunday = "0"; e.g. "13456" is every day but Monday
  // and Tuesday.
  'Days Of Week': string
  // Minutes since midnight, local time.
  'Start Minute': number
  'End Minute': number
}

export interface Campaign {
  'Advertiser ID': string
  'Campaign ID': string
  // 0 means unlimited.
  'Daily Cap': number
  'Daily Cap Served': number
  // Lower serves first; 0 means excluded from serving entirely.
  'Priority': number
  'Pass Through Rate': number
  'Metric Type': string
  'Start At': number
  'End At': number
  'Geo Targets': string[]
  'Dayparts': CampaignDaypart[]
  'Creative Sets': CampaignCreativeSet[]
}

export interface DiagnosticEntry {
  name: string
  value: string
}

export interface ConditionMatcher {
  'Creative Instance ID': string
  'Pref Path': string
  'Condition': string
  'Current Value': string
  // Matches `kConditionMatcherMatches`/`kConditionMatcherDoesNotMatch`/
  // `kConditionMatcherInvalid` in diagnostic_manager.cc.
  'Matches': 'Yes' | 'No' | 'Invalid'
}

export interface AppState {
  rewardsEnabled: boolean
  conversionUrlPatterns: ConversionUrlPattern[]
  adEvents: AdEvent[]
  confirmationQueue: ConfirmationQueueItem[]
  paymentTokens: PaymentToken[]
  nextPaymentTokenRedemptionAt: number | null
  newTabPageAdGracePeriodEndAt: number | null
  activeNotificationAdCount: number
  activeNewTabPageAdCount: number
  activeNotificationAdCampaigns: Campaign[]
  activeNewTabPageAdCampaigns: Campaign[]
  conditionMatchers: ConditionMatcher[]
  transactions: Transaction[]
  ntpSponsoredImagesComponentId: string
  ntpSponsoredImagesLoaded: boolean
  ntpSponsoredImagesManifestVersion: string
  countryResourceComponentId: string
  languageResourceComponentId: string
  dislikedAds: string[]
  likedAds: string[]
  dislikedSegments: string[]
  likedSegments: string[]
  savedAds: string[]
  adsMarkedAsInappropriate: string[]
  adHistoryRetentionPeriodDays: number
  diagnosticId: string
  isInitialized: boolean
  diagnosticEntries: DiagnosticEntry[]
  // Comes from the browser process's VariationsService, not
  // `DiagnosticManager`, so it is tracked separately from `diagnosticEntries`
  // rather than folded into that list.
  variationsCountryCode: string
  logsSupported: boolean
  verboseLoggingEnabled: boolean
  log: string
  autoRefreshEnabled: boolean
  errorsOnlyEnabled: boolean
  eventsDateRangeFilter: DateRangeFilterState
  transactionsDateRangeFilter: DateRangeFilterState
  actions: {
    loadAdsInternals: () => void
    loadDiagnostics: () => void
    clearAdsData: () => Promise<boolean>
    setDiagnosticId: (diagnosticId: string) => void
    loadLog: () => void
    clearLog: () => void
    fetchFullLog: () => Promise<string>
    toggleVerboseLoggingAndRestart: () => void
    setAutoRefreshEnabled: (enabled: boolean) => void
    setErrorsOnlyEnabled: (enabled: boolean) => void
    setEventsDateRangeFilter: (filter: DateRangeFilterState) => void
    setTransactionsDateRangeFilter: (filter: DateRangeFilterState) => void
  }
}

export type AppStore = StateStore<AppState>

export function defaultAppStore() {
  return createStateStore<AppState>({
    rewardsEnabled: false,
    conversionUrlPatterns: [],
    adEvents: [],
    confirmationQueue: [],
    paymentTokens: [],
    nextPaymentTokenRedemptionAt: null,
    newTabPageAdGracePeriodEndAt: null,
    activeNotificationAdCount: 0,
    activeNewTabPageAdCount: 0,
    activeNotificationAdCampaigns: [],
    activeNewTabPageAdCampaigns: [],
    conditionMatchers: [],
    transactions: [],
    ntpSponsoredImagesComponentId: '',
    ntpSponsoredImagesLoaded: false,
    ntpSponsoredImagesManifestVersion: '',
    countryResourceComponentId: '',
    languageResourceComponentId: '',
    dislikedAds: [],
    likedAds: [],
    likedSegments: [],
    savedAds: [],
    dislikedSegments: [],
    adsMarkedAsInappropriate: [],
    adHistoryRetentionPeriodDays: 30,
    diagnosticId: '',
    isInitialized: false,
    diagnosticEntries: [],
    variationsCountryCode: '',
    logsSupported: false,
    verboseLoggingEnabled: false,
    log: '',
    autoRefreshEnabled: false,
    errorsOnlyEnabled: true,
    eventsDateRangeFilter: { preset: 'day', fromDate: '', toDate: '' },
    transactionsDateRangeFilter: { preset: 'day', fromDate: '', toDate: '' },
    actions: {
      loadAdsInternals() {},
      loadDiagnostics() {},
      async clearAdsData() {
        return false
      },
      setDiagnosticId(diagnosticId) {},
      loadLog() {},
      clearLog() {},
      async fetchFullLog() {
        return ''
      },
      toggleVerboseLoggingAndRestart() {},
      setAutoRefreshEnabled(enabled) {},
      setErrorsOnlyEnabled(enabled) {},
      setEventsDateRangeFilter(filter) {},
      setTransactionsDateRangeFilter(filter) {},
    },
  })
}
