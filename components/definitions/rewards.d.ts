declare namespace Rewards {
  export interface ApplicationState {
    rewardsData: State | undefined
  }

  export enum Result {
    OK = 0,
    ERROR = 1,
    NO_PUBLISHER_STATE = 2,
    NO_LEDGER_STATE = 3,
    INVALID_PUBLISHER_STATE = 4,
    INVALID_LEDGER_STATE = 5,
    CAPTCHA_FAILED = 6
  }

  export type AddressesType = 'BAT' | 'BTC' | 'ETH' | 'LTC'
  export type Address = { address: string, qr: string | null }

  export interface State {
    addresses?: Record<AddressesType, Address>
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
    grant?: Grant
    reconcileStamp: number
    ui: {
      walletRecoverySuccess: boolean | null
      emptyWallet: boolean
      walletServerProblem: boolean
      modalBackup: boolean
    }
  }

  export interface ComponentProps {
    rewardsData: State
    actions: any
  }

  export interface Grant {
    promotionId?: string
    altcurrency?: string
    probi: string
    expiryTime: number
    captcha?: string
    status?: 'wrongPosition' | 'serverError' | number | null
  }

  export interface WalletProperties {
    balance: number
    choices: number[]
    probi: string
    range?: number[]
    rates?: Record<string, number>
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
}
