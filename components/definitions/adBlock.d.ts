declare namespace AdBlock {
  export interface State {
    stats: {
      adsBlockedStat?: number
      numBlocked: number
      regionalAdBlockEnabled: boolean
      regionalAdBlockTitle?: string
    }
  }

  export interface Actions {
    statsUpdated: any
  }
}
