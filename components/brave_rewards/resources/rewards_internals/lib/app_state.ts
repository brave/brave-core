/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet } from '../../shared/lib/external_wallet'
import { Optional } from '../../shared/lib/optional'

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
  externalWallet: ExternalWallet | null
  externalWalletId: string
  externalWalletAccountId: string
  rewardsEvents: RewardsEvent[]
  adDiagnosticId: string
  adDiagnosticEntries: AdDiagnosticsEntry[]
}

export type ContributionType =
  | 'auto-contribution'
  | 'one-time'
  | 'recurring'
  | 'transfer'
  | 'payment'

export type ContributionProcessor = 'uphold' | 'gemini' | 'bitflyer' | 'brave'

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

export interface AdDiagnosticsEntry {
  name: string
  value: string
}

export type Environment = 'development' | 'staging' | 'production'

export function defaultState(): AppState {
  return {
    balance: new Optional(),
    isKeyInfoSeedValid: true,
    paymentId: '',
    createdAt: new Optional(),
    declaredGeo: '',
    environment: null,
    creationEnvironment: null,
    contributions: [],
    rewardsLog: '',
    externalWallet: null,
    externalWalletId: '',
    externalWalletAccountId: '',
    rewardsEvents: [],
    adDiagnosticId: '',
    adDiagnosticEntries: [],
  }
}
