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
    WALLET_CREATED = 12,
    GRANT_NOT_FOUND = 13
  }

  export enum RewardsCategory {
    AUTO_CONTRIBUTE = 2,
    ONE_TIME_TIP = 8,
    RECURRING_TIP = 16
  }

  export interface State {
    adsData: AdsData
    autoContributeList: Publisher[]
    balance: Balance
    contributeLoad: boolean
    contributionMinTime: number
    contributionMinVisits: number
    contributionMonthly: number
    contributionNonVerified: boolean
    contributionVideos: boolean
    createdTimestamp: number | null
    currentGrant?: Grant
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
    }
    excludedList: ExcludedPublisher[]
    firstLoad: boolean | null
    grants?: Grant[]
    pendingContributions: PendingContribution[]
    pendingContributionTotal: number
    reconcileStamp: number
    recoveryKey: string
    recurringList: Publisher[]
    recurringLoad: boolean
    reports: Record<string, Report>
    tipsList: Publisher[]
    tipsLoad: boolean
    ui: {
      emptyWallet: boolean
      modalBackup: boolean
      modalRedirect: 'show' | 'hide' | 'error'
      paymentIdCheck: boolean
      walletRecoverySuccess: boolean | null
      walletServerProblem: boolean
      walletCorrupted: boolean
      walletImported: boolean
      onBoardingDisplayed?: boolean
    }
    walletCreated: boolean
    walletCreateFailed: boolean
    walletInfo: WalletProperties
  }

  export interface ComponentProps {
    rewardsData: State
    actions: any
  }

  export type GrantStatus = 'wrongPosition' | 'grantGone' | 'generalError' | 'grantAlreadyClaimed' | number | null

  export interface Grant {
    promotionId?: string
    altcurrency?: string
    probi: string
    expiryTime: number
    captcha?: string
    hint?: string
    status?: GrantStatus
    type?: string
  }

  export interface GrantResponse {
    promotionId?: string
    status?: number
    type?: string
  }

  export interface WalletProperties {
    choices: number[]
    range?: number[]
    grants?: Grant[]
  }

  export interface RecoverWallet {
    result: Result
    balance: number
    grants?: Grant[]
  }

  export interface GrantFinish {
    result: Result,
    statusCode: number,
    expiryTime: number
  }

  export enum Status {
    DEFAULT = 0,
    EXCLUDED = 1,
    INCLUDED = 2
  }

  export interface Publisher {
    publisherKey: string
    percentage: number
    verified: boolean
    excluded: Status
    url: string
    name: string
    provider: string
    favIcon: string
    id: string
    tipDate?: number
  }

  export interface ExcludedPublisher {
    id: string
    verified: boolean
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
    image: string
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

  export enum Category {
    AUTO_CONTRIBUTE = 2,
    ONE_TIME_TIP = 8,
    RECURRING_TIP = 21
  }

  export interface ContributionSaved {
    success: boolean
    category: Category
  }

  export interface PendingContribution {
    publisherKey: string
    percentage: number
    verified: boolean
    url: string
    name: string
    provider: string
    favIcon: string
    amount: number
    addedDate: string
    category: RewardsCategory
    viewingId: string
    expirationDate: string
  }

  export interface Balance {
    total: number
    rates: Record<string, number>
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
}
