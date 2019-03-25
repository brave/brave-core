declare namespace RewardsExtension {
  interface State {
    currentGrant?: GrantInfo
    currentNotification?: string
    enabledAC: boolean
    enabledMain: boolean
    notifications: Record<number, Notification>
    publishers: Record<string, Publisher>
    report: Report
    grants?: GrantInfo[]
    pendingContributionTotal: number
    walletCreated: boolean
    walletCreating: boolean
    walletCreateFailed: boolean
    walletProperties: WalletProperties
    recurringDonations: Record<string, number>[]
    donationAmounts: Record<string, number[]>
  }

  interface ApplicationState {
    rewardsPanelData: State | undefined
  }

  interface ComponentProps {
    actions: any
    rewardsPanelData: State
  }

  interface Publisher {
    excluded?: boolean
    favicon_url?: string
    publisher_key?: string
    name?: string
    percentage?: number
    provider?: string
    tabUrl?: string
    url?: string
    verified?: boolean
  }

  export interface Grant {
    altcurrency: string
    probi: string
    expiryTime: number
  }

  export type GrantStatus = 'wrongPosition' | 'grantGone' | 'generalError' | 'grantAlreadyClaimed' | number | null

  export interface GrantInfo {
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

  export interface GrantFinish {
    result: Result,
    statusCode: number,
    expiryTime: number
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

  export interface Captcha {
    image: string
    hint: string
  }

  export interface WalletProperties {
    balance: number
    probi: string
    rates: Record<string, number>
    grants?: Grant[]
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
    verified: boolean
  }

  interface ExcludedSitesChanged {
    publisher_key: string
    excluded: boolean
  }

  interface RecurringDonation {
    recurringDonations: Record<string, number>[]
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
    verified: boolean
  }
}
