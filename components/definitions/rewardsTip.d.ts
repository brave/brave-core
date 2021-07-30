declare namespace RewardsTip {
  interface State {
    publishers: Record<string, Publisher>
    parameters: RewardsParameters
    finished: boolean
    error: boolean
    currentTipAmount?: string
    currentTipRecurring?: boolean
    recurringDonations?: RecurringTips[]  // TODO(nejczdovc): migrate to tips
    reconcileStamp: number
    balance: Balance
    externalWallet?: ExternalWallet
  }

  interface ApplicationState {
    rewardsDonateData: State | undefined  // TODO(nejczdovc): migrate to tips
  }

  interface ComponentProps {
    actions: any
    rewardsDonateData: State  // TODO(nejczdovc): migrate to tips
  }

  export enum PublisherStatus {
    NOT_VERIFIED = 0,
    CONNECTED = 1,
    VERIFIED = 2
  }

  interface Publisher {
    publisherKey: string
    name: string
    title: string
    description: string
    background: string
    logo: string
    amounts: number[]
    provider: string
    links: Record<string, string>
    status: PublisherStatus
  }

  type MediaMetaData = {
    mediaType: 'github' | 'reddit' | 'twitter'
    publisherKey: string
    publisherName: string
    publisherScreenName: string
    postId: string
    postTimestamp: string
    postText: string
  }

  export interface RewardsParameters {
    rate: number
    tipChoices: number[]
    monthlyTipChoices: number[]
  }

  export interface Grant {
    promotionId?: string
    altcurrency?: string
    probi: string
    expiryTime: number
    captcha?: string
    hint?: string
    status?: 'wrongPosition' | 'serverError' | number | null
  }

  export interface RecurringTips {
    publisherKey?: string
    monthlyDate?: number
  }

  export interface Balance {
    total: number
    wallets: Record<string, number>
  }

  export type WalletType = 'anonymous' | 'uphold'

  export enum ExternalWalletStatus {
    NOT_CONNECTED = 0,
    CONNECTED = 1,
    VERIFIED = 2,
    DISCONNECTED_NOT_VERIFIED = 3,
    DISCONNECTED_VERIFIED = 4
  }

  export interface ExternalWallet {
    token: string
    address: string
    status: ExternalWalletStatus
    type: WalletType
    verifyUrl: string
    addUrl: string
    withdrawUrl: string
    userName: string
    accountUrl: string
    loginUrl: string
  }
}
