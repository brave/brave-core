declare namespace RewardsExtension {
  interface State {
    balance: Balance
    currentNotification?: string
    enabledAC: boolean
    notifications: Record<string, Notification>
    publishers: Record<string, Publisher>
    balanceReport: BalanceReport
    promotions?: Promotion[]
    pendingContributionTotal: number
    parameters: RewardsParameters
    recurringTips: Record<string, number>[]
    tipAmounts: Record<string, number[]>
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

  export enum PublisherStatus {
    NOT_VERIFIED = 0,
    CONNECTED = 1,
    UPHOLD_VERIFIED = 2,
    BITFLYER_VERIFIED = 3
  }

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
    captchaStatus: CaptchaStatus
    captchaImage?: string
    captchaId?: string
    hint?: string
    finishTitle?: string
    finishText?: string
    finishTokenTitle?: string
  }

  export interface PromotionResponse {
    result: number
    promotions: Promotion[]
  }

  export interface PromotionFinish {
    result: Result,
    promotion: Promotion
  }

  export const enum Result {
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
    WALLET_CORRUPT = 17
  }

  export interface Captcha {
    result: number
    promotionId: string
    captchaImage: string
    hint: string
  }

  export interface RewardsParameters {
    rate: number
    monthlyTipChoices: number[]
    autoContributeChoices: number[]
  }

  export interface BalanceReport {
    ads: number
    contribute: number
    monthly: number
    grant: number
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
    recurringTips: Record<string, number>[]
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
    DISCONNECTED_VERIFIED = 4,
    PENDING = 5
  }

  export interface ExternalWallet {
    token: string
    address: string
    status: WalletStatus
    type: WalletType
    verifyUrl: string
    addUrl: string
    withdrawUrl: string
    userName: string
    accountUrl: string
    loginUrl: string
  }
}
