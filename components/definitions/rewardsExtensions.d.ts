declare namespace RewardsExtension {
  interface State {
    balance: Balance
    currentPromotion?: Promotion
    currentNotification?: string
    enabledAC: boolean
    enabledMain: boolean
    notifications: Record<string, Notification>
    publishers: Record<string, Publisher>
    report: Report
    promotions?: Promotion[]
    pendingContributionTotal: number
    walletCorrupted: boolean
    walletCreated: boolean
    walletCreating: boolean
    walletCreateFailed: boolean
    recurringTips: Record<string, number>[]
    tipAmounts: Record<string, number[]>
    externalWallet?: ExternalWallet
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
    VERIFIED = 2
  }

  interface Publisher {
    excluded?: boolean
    favicon_url?: string
    publisher_key?: string
    name?: string
    percentage?: number
    provider?: string
    tabId?: number
    tabUrl?: string
    url?: string
    status?: PublisherStatus
  }

  export type PromotionStatus = 'wrongPosition' | 'grantGone' | 'generalError' | 'grantAlreadyClaimed' | number | null

  export enum PromotionTypes {
    UGP = 0,
    ADS = 1
  }

  export interface Promotion {
    promotionId?: string
    amount: number
    expiresAt: number
    captcha?: string
    hint?: string
    status?: PromotionStatus
    type: PromotionTypes
    finishTitle?: string
    finishText?: string
    finishTokenTitle?: string
  }

  export interface PromotionResponse {
    status: number
    promotionId: string
    amount: number
    expiresAt: number
    type: PromotionTypes
  }

  export interface PromotionFinish {
    result: Result,
    statusCode: number,
    expiryTime: number
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
    image: string
    hint: string
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

  export interface Notification {
    id: string
    type: number
    timestamp: number
    args: string[]
  }

  interface PublisherNormalized {
    publisher_key: string
    percentage: number
    status: PublisherStatus
  }

  interface ExcludedSitesChanged {
    publisher_key: string
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
    userName: string
    accountUrl: string
  }
}
