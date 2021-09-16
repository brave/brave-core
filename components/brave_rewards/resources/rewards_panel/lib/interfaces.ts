/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ExternalWallet, ExternalWalletProvider } from '../../shared/lib/external_wallet'
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
  monthlyContribution: number
  supportedWalletProviders: ExternalWalletProvider[]
}

export type GrantCaptchaStatus = 'pending' | 'passed' | 'failed' | 'error'

export interface GrantInfo {
  id: string
  source: 'ads' | 'ugp'
  amount: number
  expiresAt: number | null
}

export interface GrantCaptchaInfo {
  id: string
  imageURL: string
  hint: string
  status: GrantCaptchaStatus
  grantInfo: GrantInfo
}

export interface Settings {
  adsPerHour: number
  autoContributeEnabled: boolean
  autoContributeAmount: number
}

export interface Options {
  autoContributeAmounts: number[]
}

export interface HostState {
  loading: boolean
  rewardsEnabled: boolean
  balance: number
  settings: Settings
  options: Options
  grantCaptchaInfo: GrantCaptchaInfo | null
  exchangeInfo: ExchangeInfo
  earningsInfo: EarningsInfo
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
}
