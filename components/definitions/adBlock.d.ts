declare namespace AdBlock {
  export interface ApplicationState {
    adblockData: State | undefined
  }

  export interface State {
    settings: {
      customFilters: string
      regionalLists: FilterList[]
      listSubscriptions: SubscriptionInfo[]
    }
  }

  export interface FilterList {
    uuid: string
    url: string
    title: string
    supportUrl: string
    componentId: string
    base64PublicKey: string
    enabled: boolean
  }

  export interface SubscriptionInfo {
    subscription_url: string
    last_update_attempt: number
    last_successful_update_attempt: number
    enabled: boolean
  }
}
