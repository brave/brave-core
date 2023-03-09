/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

declare namespace Rewards {
  export interface ApplicationState {
    rewardsData: State | undefined
  }

  export enum Result {
    LEDGER_OK = 0,
    LEDGER_ERROR = 1,
    NO_PUBLISHER_STATE = 2,
    NO_LEDGER_STATE = 3,
    INVALID_PUBLISHER_STATE = 4,
    INVALID_LEDGER_STATE = 5,
    CAPTCHA_FAILED = 6,
    NO_PUBLISHER_LIST = 7,
    TOO_MANY_RESULTS = 8,
    NOT_FOUND = 9,
    REGISTRATION_VERIFICATION_FAILED = 10,
    BAD_REGISTRATION_RESPONSE = 11
  }

  export type AddressesType = 'BTC' | 'ETH' | 'BAT' | 'LTC'
  export type Address = { address: string, qr: string | null }

  type ConnectExternalWalletError = import('gen/brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.m.js').ConnectExternalWalletError
  type UserType = import('../brave_rewards/resources/shared/lib/user_type').UserType

  export interface State {
    userType: UserType
    adsData: AdsData
    adsHistory: AdsHistory[]
    autoContributeList: Publisher[]
    balance: import(
      '../brave_rewards/resources/shared/lib/optional'
    ).Optional<number>
    balanceReport?: BalanceReport
    contributionMinTime: number
    contributionMinVisits: number
    contributionMonthly: number
    contributionNonVerified: boolean
    contributionVideos: boolean
    currentCountryCode: string
    enabledAds: boolean
    enabledAdsMigrated: boolean
    enabledContribute: boolean
    externalWallet?: ExternalWallet
    initializing: boolean
    inlineTipsEnabled: boolean
    inlineTip: {
      twitter: boolean
      reddit: boolean
      github: boolean
    }
    isUnsupportedRegion: boolean
    excludedList: ExcludedPublisher[]
    externalWalletProviderList: string[]
    monthlyReport: MonthlyReport
    monthlyReportIds: string[]
    parameters: RewardsParameters
    promotions: Promotion[]
    pendingContributions: PendingContribution[]
    pendingContributionTotal: number
    reconcileStamp: number
    recurringList: Publisher[]
    showOnboarding: boolean | null
    tipsList: Publisher[]
    ui: {
      modalConnect: boolean
      modalRedirect: ConnectExternalWalletError
        | 'error'
        | 'hide'
        | 'show'
      modalReset: boolean
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
    transactions?: TransactionReport[]
    contributions?: ContributionReport[]
  }

  export enum ReportType {
    GRANT_UGP = 0,
    AUTO_CONTRIBUTION = 1,
    GRANT_AD = 3,
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

  export interface TransactionReport {
    amount: number
    type: ReportType
    processor: Processor
    created_at: number
  }

  export interface ContributionReport {
    amount: number
    type: ReportType
    processor: Processor
    created_at: number
    publishers: Publisher[]
  }

  export type CaptchaStatus = 'start' | 'wrongPosition' | 'generalError' | 'finished' | null

  export enum PromotionTypes {
    UGP = 0,
    ADS = 1
  }

  export enum PromotionStatus {
    ACTIVE = 0,
    ATTESTED = 1,
    FINISHED = 4,
    OVER = 5
  }

  export interface Promotion {
    promotionId: string
    amount: number
    createdAt: number
    claimableUntil: number
    expiresAt: number
    status: PromotionStatus
    type: PromotionTypes
    captchaImage?: string
    captchaId?: string
    hint?: string
    captchaStatus?: CaptchaStatus
  }

  export interface PromotionResponse {
    result: number
    promotions: Promotion[]
  }

  export interface PromotionFinish {
    result: Result,
    promotion?: Promotion
  }

  type PublisherStatus = import(
    '../../components/brave_rewards/resources/shared/lib/publisher_status'
  ).PublisherStatus

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
    grant: number
    tips: number
  }

  export interface Captcha {
    result: number
    promotionId: string
    captchaImage: string
    hint: string
  }

  export interface Subdivision {
    name: string
    code: string
  }

  export interface AdsData {
    adsEnabled: boolean
    adsPerHour: number
    adsSubdivisionTargeting: string
    automaticallyDetectedAdsSubdivisionTargeting: string
    shouldAllowAdsSubdivisionTargeting: boolean
    subdivisions: Subdivision[]
    adsUIEnabled: boolean
    adsIsSupported: boolean
    needsBrowserUpgradeToServeAds: boolean
    adsNextPaymentDate: number
    adsReceivedThisMonth: number
    adsEarningsThisMonth: number
    adsEarningsLastMonth: number
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

  export interface PendingContribution {
    id: number
    publisherKey: string
    percentage: number
    status: PublisherStatus
    url: string
    name: string
    provider: string
    favIcon: string
    amount: number
    addedDate: string
    type: RewardsType
    viewingId: string
    expirationDate: string
  }

  type WalletStatus = import('gen/brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.m.js').WalletStatus

  export type WalletType = 'uphold' | 'bitflyer' | 'gemini'

  export interface ExternalWallet {
    address: string
    status: WalletStatus
    type: WalletType
    userName?: string
    accountUrl: string
    loginUrl: string
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

  export interface AdContent {
    adType: '' | 'ad_notification' | 'new_tab_page_ad' | 'promoted_content_ad' | 'inline_content_ad'
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
}
