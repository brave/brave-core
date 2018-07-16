declare namespace AdBlock {
  export interface ApplicationState {
    adblockData: State | undefined
  }

  export interface State {
    stats: {
      adsBlockedStat?: number
      numBlocked: number
      regionalAdBlockEnabled: boolean
      regionalAdBlockTitle?: string
    }
  }
}
