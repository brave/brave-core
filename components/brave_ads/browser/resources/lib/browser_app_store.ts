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

  pageCallbackRouter.updateBraveRewardsEnabled.addListener(
    (enabled: boolean) => {
      store.update({ rewardsEnabled: enabled })
    },
  )
  API.createAdsInternalsPageHandler(
    pageCallbackRouter.$.bindNewPipeAndPassRemote(),
  )

  async function loadAdsInternals() {
    try {
      const { response } = await API.getAdsInternals()
      const { creativeSetConversions = [], adEvents = [] } = JSON.parse(response)
      store.update({
        conversionUrlPatterns: creativeSetConversions,
        adEvents,
      })
    } catch (error) {
      console.error('Error getting ads internals', error)
    }
  }

  async function loadDiagnostics() {
    try {
      const { response } = await API.getDiagnostics()
      const { diagnosticId = '', entries = [] } = JSON.parse(response)
      store.update({ diagnosticId, diagnosticEntries: entries })
    } catch (error) {
      console.error('Error getting ads diagnostics', error)
    }
  }

  loadAdsInternals()
  loadDiagnostics()

  store.update({
    actions: {
      loadAdsInternals,

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

      async loadLog() {
        if (!logsSupported) {
          return
        }
        // <if expr="enable_brave_rewards && !is_ios">
        try {
          const { log } = await LogsAPI.getLog(5000)
          store.update({ log })
        } catch (error) {
          console.error('Error getting ads-internals log', error)
        }
        // </if>
      },

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
    },
  })

  return store
}
