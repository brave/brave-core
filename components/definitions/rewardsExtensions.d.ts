/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

declare namespace RewardsExtension {
  interface State {
    balance: import(
      '../brave_rewards/resources/shared/lib/optional'
    ).Optional<number>
    currentNotification?: string | number
    enabledAC: boolean
    notifications: Record<string, Notification>
    publishers: Record<string, Publisher>
    balanceReport: BalanceReport
    pendingContributionTotal: number
    parameters: RewardsParameters
    recurringTips: Record<string, number>[]
    externalWallet?: ExternalWallet
    initializing: boolean
    showOnboarding: boolean
    adsPerHour: number
    autoContributeAmount: number
  }

  interface ApplicationState {
    rewardsPanelData: State | undefined
  }

  interface ComponentProps {
    actions: any
    rewardsPanelData: State
  }

  interface ScheduledCaptcha {
    url: string
    maxAttemptsExceeded: boolean
  }

  type PublisherStatus = import(
    '../../components/brave_rewards/resources/shared/lib/publisher_status'
  ).PublisherStatus

  interface Publisher {
    excluded?: boolean
    favIconUrl?: string
    publisherKey?: string
    name?: string
    percentage?: number
    provider?: string
    tabId?: number
    tabUrl?: string
    url?: string
    status?: PublisherStatus
  }

  export const enum Result {
    OK = 0,
    FAILED = 1,
    NO_PUBLISHER_STATE = 2,
    NO_LEGACY_STATE = 3,
    INVALID_PUBLISHER_STATE = 4,
    CAPTCHA_FAILED = 6,
    NO_PUBLISHER_LIST = 7,
    TOO_MANY_RESULTS = 8,
    NOT_FOUND = 9,
    REGISTRATION_VERIFICATION_FAILED = 10,
    BAD_REGISTRATION_RESPONSE = 11,
    WALLET_CORRUPT = 17
  }

  export type ProviderPayoutStatus = 'off' | 'processing' | 'complete'

  export interface RewardsParameters {
    rate: number
    monthlyTipChoices: number[]
    autoContributeChoices: number[]
    payoutStatus: Record<string, ProviderPayoutStatus>
    walletProviderRegions: Record<string, { allow: string[], block: string[] } | undefined>
    vbatDeadline: number | undefined
    vbatExpired: boolean
  }

  export interface BalanceReport {
    ads: number
    contribute: number
    monthly: number
    tips: number
  }

  export interface Notification {
    id: string
    type: number
    timestamp: number
    args: string[]
  }

  interface PublisherNormalized {
    publisherKey: string
    percentage: number
    status: PublisherStatus
  }

  interface ExcludedSitesChanged {
    publisherKey: string
    excluded: boolean
  }

  interface RecurringTips {
    recurringTips: ({ publisherKey: string, amount: number })[]
  }

  interface PublisherBanner {
    publisherKey: string
    name: string
    title: string
    description: string
    background: string
    logo: string
    amounts: number[],
    provider: string
    social: Record<string, string>
    status: PublisherStatus
  }

  export type TipDialogEntryPoint = 'one-time' | 'set-monthly' | 'clear-monthly'

  type WalletStatus = import('gen/brave/components/brave_rewards/common/mojom/rewards.mojom.m.js').WalletStatus

  export type WalletType = 'uphold' | 'bitflyer' | 'gemini'

  export interface ExternalWallet {
    address: string
    status: WalletStatus
    type: WalletType
    userName: string
    accountUrl: string
    activityUrl: string
  }
}
