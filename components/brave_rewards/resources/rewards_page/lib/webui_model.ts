/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import { externalWalletFromExtensionData, ExternalWalletProvider, externalWalletProviderFromString } from '../../shared/lib/external_wallet'
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
  const pageHandler = browserProxy.handler
  const stateManager = createStateManager<AppState>(defaultState())
  const isBubble = loadTimeData.getBoolean('isBubble')

  // Expose the state manager for devtools diagnostic purposes.
  Object.assign(self, {
    [Symbol.for('stateManager')]: stateManager
  })

  stateManager.update({
    embedder: {
      isBubble,
      platform: normalizePlatform(loadTimeData.getString('platform')),
      animatedBackgroundEnabled:
        loadTimeData.getBoolean('animatedBackgroundEnabled')
    }
  })

  async function updatePaymentId() {
    const { paymentId } = await pageHandler.getRewardsPaymentId()
    stateManager.update({ paymentId })
  }

  async function updateCountryCode() {
    const { countryCode } = await pageHandler.getCountryCode()
    stateManager.update({ countryCode })
  }

  async function updateExternalWallet() {
    const { externalWallet } = await pageHandler.getExternalWallet()
    stateManager.update({
      externalWallet: externalWalletFromExtensionData(externalWallet)
    })
  }

  async function updateAdsInfo() {
    const { statement } = await pageHandler.getAdsStatement()
    if (statement) {
      stateManager.update({
        adsInfo: {
          adsReceivedThisMonth: statement.adsReceivedThisMonth
        }
      })
    } else {
      stateManager.update({ adsInfo: null })
    }
  }

  async function updateRewardsParameters() {
    const { rewardsParameters } = await pageHandler.getRewardsParameters()
    stateManager.update({ rewardsParameters })
  }

  async function loadData() {
    await Promise.all([
      updatePaymentId(),
      updateCountryCode(),
      updateExternalWallet(),
      updateAdsInfo(),
      updateRewardsParameters()
    ])

    stateManager.update({ loading: false })
  }

  browserProxy.callbackRouter.onRewardsStateUpdated.addListener(() => {
    loadData()
  })

  // When displayed in a bubble, this page may be cached. In order to reset the
  // view state when the bubble is re-opened with cached contents, we update the
  // "openTime" state when the document visibility changes.
  if (isBubble) {
    document.addEventListener('visibilitychange', () => {
      const now = Date.now()
      const { openTime } = stateManager.getState()
      if (now - openTime > 100) {
        stateManager.update({ openTime: now })
      }
    })
  }

  loadData()

  return {
    getState: stateManager.getState,

    addListener: stateManager.addListener,

    onAppRendered() {
      pageHandler.onPageReady()
    },

    openTab(url) {
      if (isBubble) {
        pageHandler.openTab(url)
        return
      }
      window.open(url, '_blank', 'noopener,noreferrer')
    },

    getString(key) {
      return loadTimeData.getString(key)
    },

    async getPluralString(key, count) {
      const { pluralString } = await pageHandler.getPluralString(key, count)
      return pluralString
    },

    async enableRewards(countryCode) {
      const { result } = await pageHandler.enableRewards(countryCode)
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
      const { availableCountries } = await pageHandler.getAvailableCountries()
      return availableCountries
    },

    async getExternalWalletProviders() {
      const providers: ExternalWalletProvider[] = []
      const result = await pageHandler.getExternalWalletProviders()
      for (const key of result.providers) {
        const provider = externalWalletProviderFromString(key)
        if (provider) {
          providers.push(provider)
        }
      }
      return providers
    },

    async beginExternalWalletLogin(provider) {
      const { params } = await pageHandler.beginExternalWalletLogin(provider)
      if (!params) {
        return false
      }
      if (isBubble) {
        pageHandler.openTab(params.url)
      } else {
        window.open(params.url, '_self', 'noreferrer')
      }
      return true
    },

    async connectExternalWallet(provider, args) {
      const { result } = await pageHandler.connectExternalWallet(provider, args)
      switch (result) {
        case mojom.ConnectExternalWalletResult.kSuccess:
          return 'success'
        case mojom.ConnectExternalWalletResult.kDeviceLimitReached:
          return 'device-limit-reached'
        case mojom.ConnectExternalWalletResult.kFlaggedWallet:
          return 'flagged-wallet'
        case mojom.ConnectExternalWalletResult.kKYCRequired:
          return 'kyc-required'
        case mojom.ConnectExternalWalletResult.kMismatchedCountries:
          return 'mismatched-countries'
        case mojom.ConnectExternalWalletResult.kMismatchedProviderAccounts:
          return 'mismatched-provider-accounts'
        case mojom.ConnectExternalWalletResult.kProviderUnavailable:
          return 'provider-unavailable'
        case mojom.ConnectExternalWalletResult.kRegionNotSupported:
          return 'region-not-supported'
        case mojom.ConnectExternalWalletResult
                  .kRequestSignatureVerificationFailure:
          return 'request-signature-verification-error'
        case mojom.ConnectExternalWalletResult.kUnexpected:
          return 'unexpected-error'
        case mojom.ConnectExternalWalletResult.kUpholdBATNotAllowed:
          return 'uphold-bat-not-allowed'
        case mojom.ConnectExternalWalletResult.kUpholdInsufficientCapabilities:
          return 'uphold-insufficient-capabilities'
        case mojom.ConnectExternalWalletResult
                  .kUpholdTransactionVerificationFailure:
          return 'uphold-transaction-verification-failure'
        default:
          console.error('Unrecognized result value ' + result)
          return 'unexpected-error'
      }
    },

    async resetRewards() {
      await pageHandler.resetRewards()
    }
  }
}
