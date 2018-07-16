declare namespace NewTab {
  export interface ApplicationState {
    newTabData: State | undefined
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
    bookmarked?: Bookmark
  }

  export interface Stats {
    adsBlockedStat: number
    trackersBlockedStat: number
    javascriptBlockedStat: number
    httpsUpgradesStat: number
    fingerprintingBlockedStat: number
  }

  export interface Bookmark {
    dateAdded: number
    id: string
    index: number
    parentId: string
    title: string
    url: string
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
    bookmarks: Record<string, Bookmark>,
    stats: Stats
    backgroundImage?: Image
    gridLayoutSize?: 'small'
    showSiteRemovalNotification?: boolean
  }
}
