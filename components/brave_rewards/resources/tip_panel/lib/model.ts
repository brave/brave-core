/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Optional } from '../../shared/lib/optional'
import { ExternalWalletProvider } from '../../shared/lib/external_wallet'

export interface CreatorBanner {
  name: string
  provider: string
  title: string
  description: string
  logo: string
  background: string
  links: Record<string, string>
  web3Url: string
}

export interface RewardsUser {
  balance: Optional<number>
  walletProvider: ExternalWalletProvider | null
  walletAuthorized: boolean
  reconnectUrl: string
}

export interface RewardsParameters {
  exchangeRate: number
  exchangeCurrency: string
  contributionAmounts: number[]
}

export type AppError =
  'load-banner-error' |
  'load-parameters-error'

export interface ModelState {
  loading: boolean
  error: AppError | null
  creatorBanner: CreatorBanner
  creatorVerified: boolean
  creatorWallets: ExternalWalletProvider[]
  rewardsUser: RewardsUser
  rewardsParameters: RewardsParameters
  monthlyContributionSet: boolean
}

export type ModelStateListener = (state: ModelState) => void

export interface Model {
  getState: () => ModelState
  addListener: (callback: ModelStateListener) => () => void
  sendContribution: (amount: number, monthly: boolean) => Promise<boolean>
}

export function defaultState (): ModelState {
  return {
    loading: true,
    error: null,
    creatorBanner: {
      name: '',
      provider: '',
      title: '',
      description: '',
      logo: '',
      background: '',
      links: {},
      web3Url: ''
    },
    creatorVerified: false,
    creatorWallets: [],
    rewardsUser: {
      balance: new Optional(),
      walletProvider: null,
      walletAuthorized: false,
      reconnectUrl: ''
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
    async sendContribution (amount: number, monthly: boolean) { return false }
  }
}
