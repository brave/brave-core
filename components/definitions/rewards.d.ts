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
    BAD_REGISTRATION_RESPONSE = 11,
    WALLET_CREATED = 12
  }

  export type AddressesType = 'BTC' | 'ETH' | 'BAT' | 'LTC'
  export type Address = { address: string, qr: string | null }

  export interface State {
    addresses?: Record<AddressesType, Address>
    adsData: AdsData
    adsHistory: AdsHistoryData[]
    autoContributeList: Publisher[]
    balance: Balance
    contributeLoad: boolean
    contributionMinTime: number
    contributionMinVisits: number
    contributionMonthly: number
    contributionNonVerified: boolean
    contributionVideos: boolean
    createdTimestamp: number | null
    donationAbilityTwitter: boolean
    donationAbilityYT: boolean
    enabledAds: boolean
    enabledAdsMigrated: boolean
    enabledContribute: boolean
    enabledMain: boolean
    externalWallet?: ExternalWallet
    inlineTip: {
      twitter: boolean
      reddit: boolean
      github: boolean
    }
    excludedList: ExcludedPublisher[]
    firstLoad: boolean | null
    promotions?: Promotion[]
    pendingContributions: PendingContribution[]
    pendingContributionTotal: number
    reconcileStamp: number
    recoveryKey: string
    recurringList: Publisher[]
    recurringLoad: boolean
    reports: Record<string, Report>
    safetyNetFailed?: boolean
    tipsList: Publisher[]
    tipsLoad: boolean
    ui: {
      emptyWallet: boolean
      modalBackup: boolean
      modalRedirect: 'show' | 'hide' | 'error' | 'notAllowed'
      paymentIdCheck: boolean
      walletRecoverySuccess: boolean | null
      walletServerProblem: boolean
      walletCorrupted: boolean
      walletImported: boolean
      onBoardingDisplayed?: boolean
      onlyAnonWallet?: boolean
    }
    walletCreated: boolean
    walletCreateFailed: boolean
    walletInfo: WalletProperties
  }

  export interface ComponentProps {
    rewardsData: State
    actions: any
  }

  export type CaptchaStatus = 'start' | 'wrongPosition' | 'generalError' | 'finished' | null

  export enum PromotionTypes {
    UGP = 0,
    ADS = 1
  }

  export enum PromotionStatus {
    ACTIVE = 0,
    ATTESTED = 1,
    CLAIMED = 2,
    SIGNED_TOKENS = 3,
    FINISHED = 4,
    OVER = 5
  }

  export interface Promotion {
    promotionId: string
    amount: number
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

  export interface WalletProperties {
    choices: number[]
    userFundsPresent: boolean
  }

  export interface RecoverWallet {
    result: Result
    balance: number
  }

  export interface PromotionFinish {
    result: Result,
    promotion?: Promotion
  }

  export enum ExcludeStatus {
    DEFAULT = 0,
    EXCLUDED = 1,
    INCLUDED = 2
  }

  export enum PublisherStatus {
    NOT_VERIFIED = 0,
    CONNECTED = 1,
    VERIFIED = 2
  }

  export interface Publisher {
    publisherKey: string
    percentage: number
    status: PublisherStatus
    excluded: ExcludeStatus
    url: string
    name: string
    provider: string
    favIcon: string
    id: string
    tipDate?: number
  }

  export interface ExcludedPublisher {
    id: string
    status: PublisherStatus
    url: string
    name: string
    provider: string
    favIcon: string
  }

  export interface Report {
    ads: string
    closing: string
    contribute: string
    deposit: string
    donation: string
    grant: string
    tips: string
    opening: string
    total: string
  }

  export interface Captcha {
    result: number
    promotionId: string
    captchaImage: string
    hint: string
  }

  export interface AdsData {
    adsEnabled: boolean
    adsPerHour: number
    adsUIEnabled: boolean
    adsIsSupported: boolean
    adsEstimatedPendingRewards: number
    adsNextPaymentDate: string
    adsAdNotificationsReceivedThisMonth: number
  }

  export enum RewardsType {
    AUTO_CONTRIBUTE = 2,
    ONE_TIME_TIP = 8,
    RECURRING_TIP = 21
  }

  export interface ContributionSaved {
    success: boolean
    type: RewardsType
  }

  export interface PendingContribution {
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

  export interface Balance {
    total: number
    rates: Record<string, number>
    userFunds: string
    wallets: Record<string, number>
  }

  export type WalletType = 'anonymous' | 'uphold'

  export enum WalletStatus {
    NOT_CONNECTED = 0,
    CONNECTED = 1,
    VERIFIED = 2,
    DISCONNECTED_NOT_VERIFIED = 3,
    DISCONNECTED_VERIFIED = 4
  }

  export interface ExternalWallet {
    token: string
    address: string
    status: WalletStatus
    type: WalletType
    verifyUrl: string
    addUrl: string
    withdrawUrl: string
    userName?: string
    accountUrl: string
  }

  export interface ProcessRewardsPageUrl {
    result: number
    walletType: string
    action: string
    args: Record<string, string>
  }

  export interface AdsHistoryData {
    [key: string]: any
    id: string
    date: string
    adDetailRows: AdHistoryDetail[]
  }

  export interface AdHistoryDetail {
    id: string
    adContent: AdContent
    categoryContent: CategoryContent
  }

  export interface AdContent {
    uuid: string
    creativeSetId: string
    brand: string
    brandInfo: string
    brandLogo: string
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
    onOptInAction?: () => void
    onOptOutAction?: () => void
  }

  export interface ToggleSaveAd {
    uuid: string
    saved: boolean
  }

  export interface ToggleFlagAd {
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
