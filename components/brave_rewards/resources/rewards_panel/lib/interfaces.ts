/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  ExternalWallet,
  ExternalWalletProvider,
  ExternalWalletProviderRegionInfo
} from '../../shared/lib/external_wallet'

import { UserType } from '../../shared/lib/user_type'
import { ProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { PublisherPlatform } from '../../shared/lib/publisher_platform'
import { OnboardingResult } from '../../shared/components/onboarding'
import { ExternalWalletAction, RewardsSummaryData } from '../../shared/components/wallet_card'
import { Notification, NotificationAction } from '../../shared/components/notifications'
import { Optional } from '../../shared/lib/optional'

export interface ExchangeInfo {
  currency: string
  rate: number
}

export interface EarningsInfo {
  nextPaymentDate: number
  minEarningsThisMonth: number
  maxEarningsThisMonth: number
  minEarningsLastMonth: number
  maxEarningsLastMonth: number
}

export interface PublisherInfo {
  id: string
  name: string
  verified: boolean
  icon: string
  platform: PublisherPlatform | null
  attentionScore: number
  monthlyTip: number
  supportedWalletProviders: ExternalWalletProvider[]
}

export type AdaptiveCaptchaStatus =
  'pending' |
  'success' |
  'max-attempts-exceeded'

export type AdaptiveCaptchaResult = 'success' | 'failure' | 'error'

export interface AdaptiveCaptchaInfo {
  url: string
  status: AdaptiveCaptchaStatus
}

export interface Options {
  externalWalletRegions: Map<string, ExternalWalletProviderRegionInfo>
  vbatDeadline: number | undefined
  vbatExpired: boolean
}

type RequestedView = 'rewards-setup'

export interface HostState {
  openTime: number
  loading: boolean
  rewardsEnabled: boolean
  requestedView: RequestedView | null
  balance: Optional<number>
  options: Options
  adaptiveCaptchaInfo: AdaptiveCaptchaInfo | null
  exchangeInfo: ExchangeInfo
  earningsInfo: EarningsInfo
  payoutStatus: Record<string, ProviderPayoutStatus>
  publisherInfo: PublisherInfo | null
  publisherRefreshing: boolean
  externalWalletProviders: ExternalWalletProvider[]
  externalWallet: ExternalWallet | null
  summaryData: RewardsSummaryData
  notifications: Notification[]
  availableCountries: string[]
  defaultCountry: string
  declaredCountry: string
  userType: UserType
  publishersVisitedCount: number
  selfCustodyInviteDismissed: boolean
  isTermsOfServiceUpdateRequired: boolean
}

export type HostListener = (state: HostState) => void

export interface Host {
  state: HostState
  addListener: (callback: HostListener) => () => void
  enableRewards: (country: string) => Promise<OnboardingResult>
  openAdaptiveCaptchaSupport: () => void
  openRewardsSettings: () => void
  refreshPublisherStatus: () => void
  sendTip: () => void
  handleExternalWalletAction: (action: ExternalWalletAction) => void
  handleNotificationAction: (action: NotificationAction) => void
  dismissNotification: (notification: Notification) => void
  dismissSelfCustodyInvite: () => void
  acceptTermsOfServiceUpdate: () => void
  resetRewards: () => void
  clearAdaptiveCaptcha: () => void
  handleAdaptiveCaptchaResult: (result: AdaptiveCaptchaResult) => void
  closePanel: () => void
  onAppRendered: () => void
}
