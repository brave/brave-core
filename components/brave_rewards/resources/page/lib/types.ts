/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ConnectExternalWalletResult, WalletStatus } from 'gen/brave/components/brave_rewards/common/mojom/rewards.mojom.m.js'
import { Optional } from '../../shared/lib/optional'
import { PublisherStatus } from '../../shared/lib/publisher_status'
import { UserType } from '../../shared/lib/user_type'

export interface ApplicationState {
  rewardsData: State | undefined
}

export enum Result {
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
  BAD_REGISTRATION_RESPONSE = 11
}

export type AddressesType = 'BTC' | 'ETH' | 'BAT' | 'LTC'
export type Address = { address: string, qr: string | null }

export interface State {
  userType: UserType
  isUserTermsOfServiceUpdateRequired: boolean
  adsData: AdsData
  adsHistory: AdsHistory[]
  autoContributeList: Publisher[]
  balance: Optional<number>
  balanceReport?: BalanceReport
  contributionMinTime: number
  contributionMinVisits: number
  contributionMonthly: number
  currentCountryCode: string
  isAcSupported: boolean
  enabledContribute: boolean
  externalWallet?: ExternalWallet
  initializing: boolean
  isUnsupportedRegion: boolean
  excludedList: ExcludedPublisher[]
  externalWalletProviderList: string[]
  monthlyReport: MonthlyReport
  monthlyReportIds: string[]
  parameters: RewardsParameters
  reconcileStamp: number
  recurringList: Publisher[]
  showOnboarding: boolean | null
  tipsList: Publisher[]
  ui: {
    modalConnect: boolean
    modalConnectState: 'loading' | 'error' | ''
    modalRedirect: ConnectExternalWalletResult
      | 'error'
      | 'hide'
      | 'show'
    modalRedirectProvider: string
    modalReset: boolean
    modalAdsHistory: boolean
    adsSettings: boolean
    autoContributeSettings: boolean
    promosDismissed: {
      [key: string]: boolean
    }
  }
}

export type ProviderPayoutStatus = 'off' | 'processing' | 'complete'
export type Regions = { allow: string[], block: string[] }

export interface RewardsParameters {
  rate: number
  autoContributeChoice: number
  autoContributeChoices: number[]
  payoutStatus: Record<string, ProviderPayoutStatus>
  walletProviderRegions: Record<string, Regions | undefined>
  vbatDeadline: number | undefined
  vbatExpired: boolean
}

export interface ComponentProps {
  rewardsData: State
  actions: any
}

export interface MonthlyReport {
  month: number
  year: number
  balance?: BalanceReport
  contributions?: ContributionReport[]
}

export enum ReportType {
  AUTO_CONTRIBUTION = 1,
  TIP_RECURRING = 4,
  TIP = 5
}

export enum Processor {
  NONE = 0,
  BRAVE_TOKENS = 1,
  UPHOLD = 2,
  BITFLYER = 4,
  GEMINI = 5
}

export interface ContributionReport {
  amount: number
  type: ReportType
  processor: Processor
  created_at: number
  publishers: Publisher[]
}

export interface Publisher {
  publisherKey: string
  percentage: number
  status: PublisherStatus
  excluded: boolean
  url: string
  name: string
  provider: string
  favIcon: string
  id: string
  tipDate?: number
  weight: number
}

export interface ExternalWalletProvider {
  type: string
  name: string
}

export interface ExcludedPublisher {
  id: string
  status: PublisherStatus
  url: string
  name: string
  provider: string
  favIcon: string
}

export interface BalanceReport {
  ads: number
  contribute: number
  monthly: number
  tips: number
}

export interface Subdivision {
  name: string
  subdivision: string
}

export interface AdsData {
  adsPerHour: number
  adsSubdivisionTargeting: string
  automaticallyDetectedAdsSubdivisionTargeting: string
  shouldAllowAdsSubdivisionTargeting: boolean
  subdivisions: Subdivision[]
  adsIsSupported: boolean
  needsBrowserUpgradeToServeAds: boolean
  notificationAdsEnabled: boolean
  newTabAdsEnabled: boolean
  newsAdsEnabled: boolean
  adsNextPaymentDate: number
  adsReceivedThisMonth: number
  adTypesReceivedThisMonth: Record<string, number>
  adsMinEarningsThisMonth: number
  adsMaxEarningsThisMonth: number
  adsMinEarningsLastMonth: number
  adsMaxEarningsLastMonth: number
}

export enum RewardsType {
  AUTO_CONTRIBUTE = 2,
  ONE_TIME_TIP = 8,
  RECURRING_TIP = 16
}

export interface ContributionSaved {
  success: boolean
  type: RewardsType
}

export type WalletType = 'uphold' | 'bitflyer' | 'gemini' | 'zebpay'

export interface ExternalWallet {
  address: string
  status: WalletStatus
  type: WalletType
  userName?: string
  accountUrl: string
  activityUrl: string
}

export interface AdsHistory {
  [key: string]: any
  uuid: string
  date: string
  adDetailRows: AdHistory[]
}

export interface AdHistory {
  uuid: string
  adContent: AdContent
  categoryContent: CategoryContent
}

export type AdType =
  '' |
  'ad_notification' |
  'new_tab_page_ad' |
  'promoted_content_ad' |
  'inline_content_ad'

export interface AdContent {
  adType: AdType
  creativeInstanceId: string
  creativeSetId: string
  brand: string
  brandInfo: string
  brandDisplayUrl: string
  brandUrl: string
  likeAction: number
  adAction: 'view' | 'click' | 'dismiss' | 'landed'
  savedAd: boolean
  flaggedAd: boolean
  onThumbUpPress?: () => void
  onThumbDownPress?: () => void
  onMenuSave?: () => void
  onMenuFlag?: () => void
}

export interface CategoryContent {
  category: string
  optAction: number
  onOptIn?: () => void
  onOptOut?: () => void
}

export interface ToggleSavedAd {
  uuid: string
  saved: boolean
}

export interface ToggleFlaggedAd {
  uuid: string
  flagged: boolean
}

export interface ToggleOptAction {
  category: string
  action: number
}

export interface ToggleLikeAction {
  uuid: string
  action: number
}
