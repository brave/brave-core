declare namespace RewardsTip {
  interface State {
    publishers: Record<string, Publisher>
    walletInfo: WalletProperties
    finished: boolean
    error: boolean
    currentTipAmount?: string
    currentTipRecurring?: boolean
    recurringDonations?: RecurringTips[]  // TODO(nejczdovc): migrate to tips
    reconcileStamp: number
    balance: Balance
  }

  interface ApplicationState {
    rewardsDonateData: State | undefined  // TODO(nejczdovc): migrate to tips
  }

  interface ComponentProps {
    actions: any
    rewardsDonateData: State  // TODO(nejczdovc): migrate to tips
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

  type MediaMetaData = {
    mediaType: 'twitter',
    twitterName: string
    screenName: string
    userId: string
    tweetId: string
    tweetTimestamp: number
    tweetText: string
  } | {
    mediaType: 'reddit'
    userName: string
    postText: string
    postRelDate: string
  }

  interface RedditMetaData {
    userName: string
    postText: string
    postRelDate: string
  }

  export interface WalletProperties {
    choices: number[]
    range?: number[]
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

  export interface Balance {
    total: number
    rates: Record<string, number>
    wallets: Record<string, number>
  }
}
