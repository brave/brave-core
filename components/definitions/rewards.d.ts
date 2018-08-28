declare namespace Rewards {
  export interface ApplicationState {
    rewardsData: State | undefined
  }

  export enum Result {
    OK = 0,
    ERROR = 1
  }

  export interface State {
    createdTimestamp: number | null
    enabledMain: boolean
    enabledAds: boolean
    enabledContribute: boolean
    firstLoad: boolean | null
    walletCreated: boolean
    walletCreateFailed: boolean
    contributionMinTime: number
    contributionMinVisits: number
    contributionMonthly: number
    contributionNonVerified: boolean
    contributionVideos: boolean
    donationAbilityYT: boolean
    donationAbilityTwitter: boolean
    walletInfo: WalletProperties
    connectedWallet: boolean
    recoveryKey: string
    promotion?: Promotion
    ui: {
      walletRecoverySuccess: boolean | null
      emptyWallet: boolean
    }
  }

  export interface ComponentProps {
    rewardsData: State
    actions: any
  }

  export interface Grant {
    altcurrency: string
    probi: string
    expiryTime: number
  }

  export interface WalletProperties {
    balance: number
    choices: number[]
    probi: string
    range?: number[]
    rates?: Record<string, number>
    grants?: Grant[]
  }

  export interface Promotion {
    promotionId: string
    amount: number
    captcha?: string
    expireDate?: number
    error?: 'wrongPosition' | 'serverError'
  }

  export interface RecoverWallet {
    result: Result
    balance: number
  }
}
