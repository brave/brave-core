/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as AdsInternalsMojo from 'gen/brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.m.js'

import { AppStore, defaultAppStore } from './app_store'

const API = AdsInternalsMojo.AdsInternals.getRemote()
const pageCallbackRouter = new AdsInternalsMojo.AdsInternalsPageCallbackRouter()

export function createAppStore(): AppStore {
  const store = defaultAppStore()

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
    },
  })

  return store
}
