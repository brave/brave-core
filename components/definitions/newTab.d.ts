declare namespace NewTab {
  export interface ApplicationState {
    newTabData: State | undefined
  }

  export interface Image {
    name: string
    source: string
    author: string
    link: string
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

  export interface PersistentState {
    topSites: Site[]
    ignoredTopSites: Site[]
    pinnedTopSites: Site[]
    gridSites: Site[]
    showEmptyPage: boolean
    bookmarks: Record<string, Bookmark>
  }

  export interface EphemeralState {
    initialDataLoaded: boolean
    isIncognito: boolean
    useAlternativePrivateSearchEngine: boolean
    isTor: boolean
    isQwant: boolean
    backgroundImage?: Image
    gridLayoutSize?: 'small'
    showSiteRemovalNotification?: boolean
    showBackgroundImage: boolean
    showSettings: boolean
    stats: Stats
  }

  // In-memory state is a superset of PersistentState
  export type State = PersistentState & EphemeralState
}
