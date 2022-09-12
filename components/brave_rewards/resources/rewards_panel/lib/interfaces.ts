/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ExternalWallet, ExternalWalletProvider } from '../../shared/lib/external_wallet'
import { GrantInfo } from '../../shared/lib/grant_info'
import { ProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { PublisherPlatform } from '../../shared/lib/publisher_platform'
import { ExternalWalletAction, RewardsSummaryData } from '../../shared/components/wallet_card'
import { Notification, NotificationAction } from '../../shared/components/notifications'

export interface ExchangeInfo {
  currency: string
  rate: number
}

export interface EarningsInfo {
  nextPaymentDate: number
  earningsThisMonth: number
  earningsLastMonth: number
}

export interface PublisherInfo {
  id: string
  name: string
  icon: string
  platform: PublisherPlatform | null
  registered: boolean
  attentionScore: number
  autoContributeEnabled: boolean
  monthlyTip: number
  supportedWalletProviders: ExternalWalletProvider[]
}

export type GrantCaptchaStatus = 'pending' | 'passed' | 'failed' | 'error'

export interface GrantCaptchaInfo {
  id: string
  imageURL: string
  hint: string
  status: GrantCaptchaStatus
  verifying: boolean
  grantInfo: GrantInfo
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

export interface Settings {
  adsPerHour: number
  autoContributeEnabled: boolean
  autoContributeAmount: number
}

export interface Options {
  autoContributeAmounts: number[]
}

type RequestedView = 'rewards-tour'

export interface HostState {
  openTime: number
  loading: boolean
  requestedView: RequestedView | null
  rewardsEnabled: boolean
  balance: number
  settings: Settings
  options: Options
  grantCaptchaInfo: GrantCaptchaInfo | null
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
}

export type HostListener = (state: HostState) => void

export type MonthlyTipAction = 'update' | 'cancel'

export interface Host {
  state: HostState
  addListener: (callback: HostListener) => () => void
  getString: (key: string) => string
  enableRewards: () => void
  openAdaptiveCaptchaSupport: () => void
  openRewardsSettings: () => void
  refreshPublisherStatus: () => void
  setIncludeInAutoContribute: (include: boolean) => void
  setAutoContributeAmount: (amount: number) => void
  setAdsPerHour: (adsPerHour: number) => void
  sendTip: () => void
  handleMonthlyTipAction: (action: MonthlyTipAction) => void
  handleExternalWalletAction: (action: ExternalWalletAction) => void
  handleNotificationAction: (action: NotificationAction) => void
  dismissNotification: (notification: Notification) => void
  solveGrantCaptcha: (solution: { x: number, y: number }) => void
  clearGrantCaptcha: () => void
  clearAdaptiveCaptcha: () => void
  handleAdaptiveCaptchaResult: (result: AdaptiveCaptchaResult) => void
  onAppRendered: () => void
}
