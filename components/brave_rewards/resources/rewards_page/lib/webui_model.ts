/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import { AppModel, AppState, defaultState } from './app_model'
import { RewardsPageProxy } from './rewards_page_proxy'
import { createStateManager } from '../../shared/lib/state_manager'
import * as mojom from './mojom'

function normalizePlatform(name: string) {
  switch (name) {
    case 'android':
    case 'desktop':
      return name
  }
  console.error('Invalid platform name: ' + name)
  return 'desktop'
}

export function createModel(): AppModel {
  const browserProxy = RewardsPageProxy.getInstance()
  const stateManager = createStateManager<AppState>(defaultState())

  // Expose the state manager for devtools diagnostic purposes.
  Object.assign(self, {
    [Symbol.for('stateManager')]: stateManager
  })

  stateManager.update({
    embedder: {
      isBubble: loadTimeData.getBoolean('isBubble'),
      platform: normalizePlatform(loadTimeData.getString('platform')),
      animatedBackgroundEnabled:
        loadTimeData.getBoolean('animatedBackgroundEnabled')
    }
  })

  async function updatePaymentId() {
    const { paymentId } = await browserProxy.handler.getRewardsPaymentId()
    stateManager.update({ paymentId })
  }

  async function loadData() {
    await Promise.all([
      updatePaymentId()
    ])

    stateManager.update({ loading: false })
  }

  browserProxy.callbackRouter.onRewardsStateUpdated.addListener(() => {
    loadData()
  })

  document.addEventListener('visibilitychange', () => {
    const now = Date.now()
    const { openTime } = stateManager.getState()
    if (now - openTime > 100) {
      stateManager.update({ openTime: now })
    }
  })

  loadData()

  return {
    getState: stateManager.getState,

    addListener: stateManager.addListener,

    onAppRendered() {
      browserProxy.handler.onPageReady()
    },

    openTab(url: string) {
      if (stateManager.getState().embedder.isBubble) {
        browserProxy.handler.openTab(url)
        return
      }
      window.open(url, '_blank', 'noopener,noreferrer')
    },

    async enableRewards(countryCode) {
      const { result } = await browserProxy.handler.enableRewards(countryCode)
      switch (result) {
        case mojom.CreateRewardsWalletResult.kSuccess:
          updatePaymentId()
          return 'success'
        case mojom.CreateRewardsWalletResult.kWalletGenerationDisabled:
          return 'wallet-generation-disabled'
        case mojom.CreateRewardsWalletResult.kGeoCountryAlreadyDeclared:
          return 'country-already-declared'
        case mojom.CreateRewardsWalletResult.kUnexpected:
          return 'unexpected-error'
        default:
          console.error('Unrecognized result value ' + result)
          return 'unexpected-error'
      }
    },

    async getAvailableCountries() {
      const { availableCountries } =
        await browserProxy.handler.getAvailableCountries()
      return availableCountries
    },

    async resetRewards() {
      await browserProxy.handler.resetRewards()
    }
  }
}
