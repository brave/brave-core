declare namespace AdBlock {
  export interface ApplicationState {
    adblockData: State | undefined
  }

  export interface State {
    settings: {
      customFilters: string
    },
    stats: {
      adsBlockedStat?: number
      numBlocked: number
      regionalAdBlockEnabled: boolean
      regionalAdBlockTitle?: string
    }
  }
}
