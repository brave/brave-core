/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Optional } from '../../shared/lib/optional'
import { ExternalWalletProvider } from '../../shared/lib/external_wallet'

export interface CreatorBanner {
  title: string
  description: string
  logo: string
  background: string
  links: Record<string, string>
  web3URL: string
}

export interface RewardsUser {
  balance: Optional<number>
  walletProvider: ExternalWalletProvider | null
  walletAuthorized: boolean
}

export interface RewardsParameters {
  exchangeRate: number
  exchangeCurrency: string
  contributionAmounts: number[]
}

export interface ModelState {
  loading: boolean
  creatorBanner: CreatorBanner
  creatorWallets: ExternalWalletProvider[]
  rewardsUser: RewardsUser
  rewardsParameters: RewardsParameters
  monthlyContributionSet: boolean
}

export type ModelStateListener = (state: ModelState) => void

export interface Model {
  getState: () => ModelState
  addListener: (callback: ModelStateListener) => () => void
  onInitialRender: () => void
  sendContribution: (amount: number, monthly: boolean) => Promise<void>
  reconnectWallet: () => void
  shareContribution: () => void
}

export function defaultState (): ModelState {
  return {
    loading: true,
    creatorBanner: {
      title: '',
      description: '',
      logo: '',
      background: '',
      links: {},
      web3URL: ''
    },
    creatorWallets: [],
    rewardsUser: {
      balance: new Optional(),
      walletProvider: null,
      walletAuthorized: false
    },
    rewardsParameters: {
      exchangeRate: 1,
      exchangeCurrency: 'USD',
      contributionAmounts: [1, 5, 10]
    },
    monthlyContributionSet: false
  }
}

export function defaultModel (): Model {
  const state = defaultState()
  return {
    getState () { return state },
    addListener () { return () => {} },
    onInitialRender () {},
    async sendContribution (amount: number, monthly: boolean) {},
    reconnectWallet () {},
    shareContribution () {}
  }
}
