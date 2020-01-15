declare namespace Playlists {
  export interface ApplicationState {
    playlistsData: State | undefined
  }

  export interface State {
    settings: {
      customFilters: string
      regionalLists: FilterList[]
    },
    stats: {
      adsBlockedStat?: number
      numBlocked: number
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
