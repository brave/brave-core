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

  export interface State {
    addresses?: Record<AddressesType, Address>
    adsData: AdsData
    adsHistory: AdsHistory[]
    autoContributeList: Publisher[]
    balance: Balance
    balanceReport?: BalanceReport
    contributeLoad: boolean
    contributionMinTime: number
    contributionMinVisits: number
    contributionMonthly: number
    contributionNonVerified: boolean
    contributionVideos: boolean
    createdTimestamp: number | null
    currentCountryCode: string
    donationAbilityTwitter: boolean
    donationAbilityYT: boolean
    enabledAds: boolean
    enabledAdsMigrated: boolean
    enabledContribute: boolean
    externalWallet?: ExternalWallet
    initializing: boolean
    inlineTip: {
      twitter: boolean
      reddit: boolean
      github: boolean
    }
    excludedList: ExcludedPublisher[]
    firstLoad: boolean | null
    monthlyReport: MonthlyReport
    monthlyReportIds: string[]
    parameters: RewardsParameters
    paymentId: string
    promotions?: Promotion[]
    pendingContributions: PendingContribution[]
    pendingContributionTotal: number
    reconcileStamp: number
    recoveryKey: string
    recurringList: Publisher[]
    recurringLoad: boolean
    safetyNetFailed?: boolean
    showOnboarding?: boolean
    tipsList: Publisher[]
    tipsLoad: boolean
    ui: {
      emptyWallet: boolean
      modalBackup: boolean
      modalRedirect: 'show' | 'hide' | 'error' | 'notAllowed' | 'batLimit'
      paymentIdCheck: boolean
      promosDismissed?: {
        [key: string]: boolean
      }
      walletRecoveryStatus: number | null
      walletServerProblem: boolean
      verifyOnboardingDisplayed?: boolean
      onlyAnonWallet?: boolean
    }
  }

  export interface RewardsParameters {
    rate: number
    autoContributeChoice: number
    autoContributeChoices: number[]
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
    BRAVE_USER_FUNDS = 3,
    BITFLYER = 4
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
    UPHOLD_VERIFIED = 2,
    BITFLYER_VERIFIED = 3
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
    weight: number
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

  export interface AdsData {
    adsEnabled: boolean
    adsPerHour: number
    adsSubdivisionTargeting: string
    automaticallyDetectedAdsSubdivisionTargeting: string
    shouldAllowAdsSubdivisionTargeting: boolean
    adsUIEnabled: boolean
    adsIsSupported: boolean
    adsEstimatedPendingRewards: number
    adsNextPaymentDate: string
    adsReceivedThisMonth: number
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

  export interface Balance {
    total: number
    wallets: Record<string, number>
  }

  export type WalletType = 'anonymous' | 'uphold' | 'bitflyer'

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
    loginUrl: string
  }

  export interface ProcessRewardsPageUrl {
    result: number
    walletType: string
    action: string
    args: Record<string, string>
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
    creativeInstanceId: string
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
