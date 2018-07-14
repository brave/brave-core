declare namespace NewTab {
  export interface ApplicationState {
    newTabData: State
  }

  export interface Image {
    name: string,
    source: string,
    author: string,
    link: string,
    style?: {
      backgroundImage: string
    }
  }

  export interface Site {
    index: number
    url: string
    title: string
    favicon: string
    letter: string
    thumb: string
    themeColor: string
    computedThemeColor: string
    pinned: boolean
    bookmarked?: boolean
  }

  export interface Stats {
    adsBlockedStat: number
    trackersBlockedStat: number
    javascriptBlockedStat: number
    httpsUpgradesStat: number
    fingerprintingBlockedStat: number
  }

  export interface State {
    imageLoadFailed: boolean
    topSites: Site[],
    ignoredTopSites: Site[],
    pinnedTopSites: Site[],
    gridSites: Site[],
    showImages: boolean,
    showEmptyPage: boolean,
    isIncognito: boolean,
    useAlternativePrivateSearchEngine: boolean,
    bookmarks: Record<string, boolean>,
    stats: Stats
    backgroundImage?: Image
    gridLayoutSize?: 'small'
    showSiteRemovalNotification?: boolean
  }

  export interface Actions {
    statsUpdated: any
  }
}
