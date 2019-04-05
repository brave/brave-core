declare namespace RewardsDonate {
  interface State {
    publishers: Record<string, Publisher>
    walletInfo: WalletProperties
    finished: boolean
    error: boolean
    currentTipAmount?: string
    currentTipRecurring?: boolean
    recurringDonations?: RecurringTips[]  // TODO(nejczdovc): migrate to tips
    reconcileStamp: number
  }

  interface ApplicationState {
    rewardsDonateData: State | undefined
  }

  interface ComponentProps {
    actions: any
    rewardsDonateData: State
  }

  interface Publisher {
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

  export interface WalletProperties {
    balance: number
    choices: number[]
    probi: string
    range?: number[]
    rates?: Record<string, number>
    grants?: Grant[]
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
}
