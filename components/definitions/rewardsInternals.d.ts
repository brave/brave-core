/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

declare namespace RewardsInternals {
  export interface ApplicationState {
    rewardsInternalsData: State | undefined
  }

  export interface State {
    balance: import(
      'gen/brave/components/brave_rewards/common/mojom/rewards.mojom.m.js'
    ).Balance
    info: {
      isKeyInfoSeedValid: boolean
      walletPaymentId: string
      bootStamp: number
      declaredGeo: string
      walletCreationEnvironment?: Environment
    }
    contributions: ContributionInfo[]
    log: string
    fullLog: string
    externalWallet?: ExternalWallet
    eventLogs: EventLog[]
    adDiagnostics: AdDiagnostics
    environment: Environment
  }

  export interface ContributionInfo {
    id: string
    amount: number
    type: number
    step: number
    retryCount: number
    createdAt: number
    processor: number
    publishers: ContributionPublisher[]
  }

  export interface ContributionPublisher {
    contributionId: string
    publisherKey: string
    totalAmount: number
    contributedAmount: number
  }

  type WalletStatus = import('gen/brave/components/brave_rewards/common/mojom/rewards_types.mojom.m.js').WalletStatus

  export type WalletType = 'uphold' | 'bitflyer' | 'gemini'

  export interface ExternalWallet {
    address: string
    memberId: string
    status: WalletStatus
    type: WalletType | ''
  }

  export interface EventLog {
    id: string
    key: string
    value: string
    createdAt: number
  }

  export interface AdDiagnosticsEntry {
    name: string
    value: string
  }

  export interface AdDiagnostics {
    entries: AdDiagnosticsEntry[]
    diagnosticId: string
  }

  type Environment = import('gen/brave/components/brave_rewards/common/mojom/rewards_types.mojom.m.js').Environment
}
