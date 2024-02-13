/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Model, ModelState, AppError, defaultState } from './model'
import { TipPanelProxy } from './tip_panel_proxy'
import { createStateManager } from '../../shared/lib/state_manager'
import { ExternalWalletProvider, externalWalletProviderFromString } from '../../shared/lib/external_wallet'
import { optional } from '../../shared/lib/optional'
import { reconnectURL } from '../../shared/lib/rewards_urls'

import { PublisherStatus, WalletStatus } from 'gen/brave/components/brave_rewards/common/mojom/rewards.mojom.m'

export function createModel (): Model {
  const stateManager = createStateManager<ModelState>(defaultState())
  const proxy = TipPanelProxy.getInstance()

  self[Symbol.for('stateManager')] = stateManager

  async function loadData () {
    const [
      { balance },
      { banner },
      { externalWallet },
      { parameters },
      { monthlyContributionSet }
    ] = await Promise.all([
      proxy.handler.getBalance(),
      proxy.handler.getBanner(),
      proxy.handler.getExternalWallet(),
      proxy.handler.getRewardsParameters(),
      proxy.handler.getMonthlyContributionSet()
    ])

    function getError (): AppError | null {
      if (!banner) { return 'load-banner-error' }
      if (!parameters) { return 'load-parameters-error' }
      return null
    }

    function mapBalance () {
      if (!balance) {
        return optional<number>(undefined)
      }
      if (externalWallet && externalWallet.status === WalletStatus.kLoggedOut) {
        return optional<number>(undefined)
      }
      return optional(balance.total)
    }

    function mapWalletProvider () {
      return externalWallet
        ? externalWalletProviderFromString(externalWallet.type)
        : null
    }

    function mapWalletAuthorized () {
      return externalWallet
        ? externalWallet.status === WalletStatus.kConnected
        : false
    }

    function mapURL (url: string) {
      try {
        return new URL(url).toString()
      } catch {
        return ''
      }
    }

    function mapBanner () {
      if (!banner) {
        return {
          name: '',
          provider: '',
          title: '',
          description: '',
          logo: '',
          background: '',
          links: {},
          web3Url: ''
        }
      }

      banner.web3Url = mapURL(banner.web3Url)
      for (const [key, value] of Object.entries(banner.links)) {
        banner.links[key] = mapURL(String(value || ''))
      }

      return banner
    }

    function mapCreatorVerified () {
      return Boolean(banner && banner.status !== PublisherStatus.NOT_VERIFIED)
    }

    function mapCreatorWallets (): ExternalWalletProvider[] {
      if (!banner) {
        return []
      }
      switch (banner.status) {
        case PublisherStatus.BITFLYER_VERIFIED: return ['bitflyer']
        case PublisherStatus.GEMINI_VERIFIED: return ['gemini']
        case PublisherStatus.UPHOLD_VERIFIED: return ['uphold']
      }
      return []
    }

    stateManager.update({
      loading: false,
      error: getError(),
      rewardsUser: {
        balance: mapBalance(),
        walletProvider: mapWalletProvider(),
        walletAuthorized: mapWalletAuthorized(),
        reconnectUrl: reconnectURL
      },
      rewardsParameters: {
        exchangeCurrency: 'USD',
        exchangeRate: parameters ? parameters.rate : 1,
        contributionAmounts: parameters ? parameters.tipChoices : []
      },
      creatorBanner: mapBanner(),
      creatorVerified: mapCreatorVerified(),
      creatorWallets: mapCreatorWallets(),
      monthlyContributionSet
    })
  }

  loadData()

  return {
    getState: stateManager.getState,
    addListener: stateManager.addListener,
    async sendContribution (amount: number, monthly: boolean) {
      const result = await proxy.handler.sendContribution(amount, monthly)
      return result.success
    }
  }
}
