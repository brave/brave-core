declare namespace RewardsDonate {
  interface State {
    publisher?: Publisher
    walletInfo: WalletProperties
    finished: boolean
    error: boolean
    recurringList?: string[]
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
    amount: number[],
    social: Record<string, string>
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
}
