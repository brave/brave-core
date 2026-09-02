/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import * as AdsInternalsMojo from 'gen/brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.m.js'

import { AppStore, defaultAppStore } from './app_store'

const API = AdsInternalsMojo.AdsInternals.getRemote()
// <if expr="enable_brave_rewards && !is_ios">
const LogsAPI = AdsInternalsMojo.AdsInternalsLogs.getRemote()
// </if>
const pageCallbackRouter = new AdsInternalsMojo.AdsInternalsPageCallbackRouter()

export function createAppStore(): AppStore {
  const store = defaultAppStore()
  const logsSupported = loadTimeData.getBoolean('logsSupported')
  store.update({
    logsSupported,
    verboseLoggingEnabled: loadTimeData.getBoolean('verboseLoggingEnabled'),
  })

  // Rewards enabled/wallet connected can change from another tab (e.g.
  // brave://rewards) while this one is open; refresh so eligibility-facing
  // data (e.g. permission rules, payment tokens) doesn't go stale.
  pageCallbackRouter.updateBraveRewardsEnabled.addListener(
    (enabled: boolean) => {
      store.update({ rewardsEnabled: enabled })
      loadAdsInternals()
      loadDiagnostics()
    },
  )
  pageCallbackRouter.updateBraveRewardsWalletConnected.addListener(() => {
    loadAdsInternals()
    loadDiagnostics()
  })
  // Ads finish initializing asynchronously after startup, so "Ads
  // initialized" and permission rule state (both diagnostics) can still be
  // stale even if this page loaded first; a plain state refresh here doesn't
  // touch scroll position.
  pageCallbackRouter.updateDidInitializeAdsService.addListener(() => {
    loadDiagnostics()
  })
  API.createAdsInternalsPageHandler(
    pageCallbackRouter.$.bindNewPipeAndPassRemote(),
  )

  // Guards against overlapping requests; the manual Refresh button on every
  // tab and the auto-refresh interval can otherwise both trigger this while a
  // prior mojo round trip is still in flight.
  let loadAdsInternalsInFlight = false

  async function loadAdsInternals() {
    if (loadAdsInternalsInFlight) {
      return
    }
    loadAdsInternalsInFlight = true
    try {
      const { response } = await API.getAdsInternals()
      const {
        creativeSetConversions = [],
        adEvents = [],
        confirmationQueue = [],
        paymentTokens = [],
        nextPaymentTokenRedemptionAt = null,
        newTabPageAdGracePeriodEndAt = null,
        activeNotificationAdCount = 0,
        activeNewTabPageAdCount = 0,
        activeNotificationAdCampaigns = [],
        activeNewTabPageAdCampaigns = [],
        conditionMatchers = [],
        transactions = [],
        dislikedAds = [],
        likedAds = [],
        dislikedSegments = [],
        likedSegments = [],
        savedAds = [],
        adsMarkedAsInappropriate = [],
        adHistoryRetentionPeriodDays = 30,
      } = JSON.parse(response)
      store.update({
        conversionUrlPatterns: creativeSetConversions,
        adEvents,
        confirmationQueue,
        paymentTokens,
        nextPaymentTokenRedemptionAt,
        newTabPageAdGracePeriodEndAt,
        activeNotificationAdCount,
        activeNewTabPageAdCount,
        activeNotificationAdCampaigns,
        activeNewTabPageAdCampaigns,
        conditionMatchers,
        transactions,
        dislikedAds,
        likedAds,
        dislikedSegments,
        likedSegments,
        savedAds,
        adsMarkedAsInappropriate,
        adHistoryRetentionPeriodDays,
      })
    } catch (error) {
      console.error('Error getting ads internals', error)
    } finally {
      loadAdsInternalsInFlight = false
    }
  }

  let loadDiagnosticsInFlight = false

  async function loadDiagnostics() {
    if (loadDiagnosticsInFlight) {
      return
    }
    loadDiagnosticsInFlight = true
    try {
      const { response } = await API.getDiagnostics()
      const {
        diagnosticId = '',
        isInitialized = false,
        entries = [],
        variationsCountryCode = '',
        ntpSponsoredImagesComponentId = '',
        ntpSponsoredImagesLoaded = false,
        ntpSponsoredImagesManifestVersion = '',
        countryResourceComponentId = '',
        languageResourceComponentId = '',
      } = JSON.parse(response)

      store.update({
        diagnosticId,
        isInitialized,
        diagnosticEntries: entries,
        variationsCountryCode,
        ntpSponsoredImagesComponentId,
        ntpSponsoredImagesLoaded,
        ntpSponsoredImagesManifestVersion,
        countryResourceComponentId,
        languageResourceComponentId,
      })
    } catch (error) {
      console.error('Error getting ads diagnostics', error)
    } finally {
      loadDiagnosticsInFlight = false
    }
  }

  async function loadLog() {
    if (!logsSupported) {
      return
    }
    // <if expr="enable_brave_rewards && !is_ios">
    try {
      // Comfortably more than the Logs tab's own render cap (see
      // `MAX_RENDERED_LINES` in logs.tsx), so filtering to Errors only still
      // has enough fetched history to find more than a screenful of errors.
      const { log } = await LogsAPI.getLog(5000)
      store.update({ log })
    } catch (error) {
      console.error('Error getting ads-internals log', error)
    }
    // </if>
  }

  loadAdsInternals()
  loadDiagnostics()

  const AUTO_REFRESH_INTERVAL_MS = 5000
  let autoRefreshInterval: number | undefined
  // Guards against overlapping ticks; a slow mojo round trip (e.g. a large
  // log fetch) could otherwise still be in flight when the next interval
  // fires, stacking redundant requests and DOM updates on top of each other.
  let refreshInFlight = false

  async function refreshAll() {
    if (refreshInFlight) {
      return
    }
    refreshInFlight = true
    try {
      await Promise.all([loadAdsInternals(), loadDiagnostics(), loadLog()])
    } finally {
      refreshInFlight = false
    }
  }

  store.update({
    actions: {
      loadAdsInternals,
      loadDiagnostics,

      async clearAdsData() {
        try {
          const { success } = await API.clearAdsData()
          if (success) {
            loadAdsInternals()
            loadDiagnostics()
          } else {
            console.warn('Failed to clear ads data')
          }
          return success
        } catch (error) {
          console.error('Error clearing ads data', error)
          return false
        }
      },

      setDiagnosticId(diagnosticId) {
        API.setDiagnosticId(diagnosticId)
        store.update({ diagnosticId })
      },

      loadLog,

      async clearLog() {
        if (!logsSupported) {
          return
        }
        // <if expr="enable_brave_rewards && !is_ios">
        try {
          const { success } = await LogsAPI.clearLog()
          if (success) {
            store.update({ log: '' })
          } else {
            console.warn('Failed to clear log')
          }
        } catch (error) {
          console.error('Error clearing ads-internals log', error)
        }
        // </if>
      },

      async fetchFullLog() {
        if (!logsSupported) {
          return ''
        }
        // <if expr="enable_brave_rewards && !is_ios">
        try {
          const { log } = await LogsAPI.getLog(null)
          return log
        } catch (error) {
          console.error('Error fetching full ads-internals log', error)
        }
        // </if>
        return ''
      },

      toggleVerboseLoggingAndRestart() {
        if (!logsSupported) {
          return
        }
        // <if expr="enable_brave_rewards && !is_ios">
        LogsAPI.toggleVerboseLoggingAndRestart()
        // </if>
      },

      setAutoRefreshEnabled(enabled) {
        if (autoRefreshInterval !== undefined) {
          clearInterval(autoRefreshInterval)
          autoRefreshInterval = undefined
        }
        if (enabled) {
          autoRefreshInterval =
            setInterval(refreshAll, AUTO_REFRESH_INTERVAL_MS) as any
        }
        store.update({ autoRefreshEnabled: enabled })
      },

      setErrorsOnlyEnabled(enabled) {
        store.update({ errorsOnlyEnabled: enabled })
      },

      setEventsDateRangeFilter(filter) {
        store.update({ eventsDateRangeFilter: filter })
      },

      setTransactionsDateRangeFilter(filter) {
        store.update({ transactionsDateRangeFilter: filter })
      },

      async testConditionMatcher(prefPath, condition, testValue) {
        try {
          const { currentValue, matches } =
            await API.evaluateConditionMatcher(
              prefPath, condition, testValue)
          return { currentValue, matches }
        } catch (error) {
          console.error('Error testing condition matcher', error)
          return { currentValue: 'Unknown', matches: 'N/A' }
        }
      },
    },
  })

  return store
}
