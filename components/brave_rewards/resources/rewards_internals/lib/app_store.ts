/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StateStore, createStateStore } from '$web-common/state_store'
import { ExternalWallet } from '../../shared/lib/external_wallet'
import { Optional } from '../../shared/lib/optional'
import { StringKey } from './locale_strings'

export interface AppState {
  balance: Optional<number>
  isKeyInfoSeedValid: boolean
  paymentId: string
  createdAt: Optional<number>
  declaredGeo: string
  environment: Environment | null
  creationEnvironment: Environment | null
  contributions: ContributionInfo[]
  rewardsLog: string
  verboseLoggingEnabled: boolean
  externalWallet: ExternalWallet | null
  externalWalletId: string
  externalWalletAccountId: string
  rewardsEvents: RewardsEvent[]
  actions: {
    getString: (key: StringKey) => string
    clearRewardsLog: () => void
    loadRewardsLog: () => void
    fetchFullRewardsLog: () => Promise<string>
    toggleVerboseLoggingAndRestart: () => void
    loadContributions: () => void
    loadRewardsEvents: () => void
  }
}

export type ContributionType =
  | 'auto-contribution'
  | 'one-time'
  | 'recurring'
  | 'transfer'
  | 'payment'

export type ContributionProcessor = 'uphold' | 'bitflyer' | 'brave'

export interface ContributionInfo {
  id: string
  amount: number
  type: ContributionType
  step: number
  retryCount: number
  createdAt: number
  processor: ContributionProcessor | null
  publishers: ContributionPublisher[]
}

export interface ContributionPublisher {
  id: string
  totalAmount: number
  contributedAmount: number
}

export interface RewardsEvent {
  id: string
  key: string
  value: string
  createdAt: number
}

export type Environment = 'development' | 'staging' | 'production'

export type AppStore = StateStore<AppState>

export function defaultAppStore() {
  return createStateStore<AppState>({
    balance: new Optional(),
    isKeyInfoSeedValid: true,
    paymentId: '',
    createdAt: new Optional(),
    declaredGeo: '',
    environment: null,
    creationEnvironment: null,
    contributions: [],
    rewardsLog: '',
    verboseLoggingEnabled: false,
    externalWallet: null,
    externalWalletId: '',
    externalWalletAccountId: '',
    rewardsEvents: [],
    actions: {
      getString(key) {
        return ''
      },
      clearRewardsLog() {},
      loadRewardsLog() {},
      async fetchFullRewardsLog() {
        return ''
      },
      toggleVerboseLoggingAndRestart() {},
      loadContributions() {},
      loadRewardsEvents() {},
    },
  })
}
