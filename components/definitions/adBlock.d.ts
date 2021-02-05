declare namespace AdBlock {
  export interface ApplicationState {
    adblockData: State | undefined
  }

  export interface State {
    settings: {
      customFilters: string
      regionalLists: FilterList[]
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
}
