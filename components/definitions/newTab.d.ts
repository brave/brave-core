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
    rewardsState: RewardsWidgetState
  }

  export interface EphemeralState {
    initialDataLoaded: boolean
    textDirection: string
    isIncognito: boolean
    useAlternativePrivateSearchEngine: boolean
    isTor: boolean
    isQwant: boolean
    backgroundImage?: Image
    gridLayoutSize?: 'small'
    showSiteRemovalNotification?: boolean
    showBackgroundImage: boolean
    showStats: boolean
    showClock: boolean
    showTopSites: boolean
    showRewards: boolean
    stats: Stats
  }

  export interface RewardsWidgetState {
    adsEstimatedEarnings: number
    adsSupported?: boolean
    balance: RewardsBalance
    dismissedNotifications: string[]
    enabledAds: boolean
    enabledMain: boolean
    grants: GrantRecord[]
    onlyAnonWallet: boolean
    totalContribution: string
    walletCreated: boolean
    walletCreating: boolean
    walletCreateFailed: boolean
    walletCorrupted: boolean
  }

  export const enum RewardsResult {
    LEDGER_OK = 0,
    LEDGER_ERROR = 1,
    NO_PUBLISHER_STATE = 2,
    NO_LEDGER_STATE = 3,
    INVALID_PUBLISHER_STATE = 4,
    INVALID_LEDGER_STATE = 5,
    CAPTCHA_FAILED = 6,
    NO_PUBLISHER_LIST = 7,
    TOO_MANY_RESULTS = 8,
    NOT_FOUND = 9,
    REGISTRATION_VERIFICATION_FAILED = 10,
    BAD_REGISTRATION_RESPONSE = 11,
    WALLET_CREATED = 12,
    GRANT_NOT_FOUND = 13,
    WALLET_CORRUPT = 17
  }

  export interface RewardsReport {
    ads: string
    closing: string
    contribute: string
    deposit: string
    donation: string
    grant: string
    tips: string
    opening: string
    total: string
  }

  export interface GrantResponse {
    promotionId?: string
    status?: number
    type?: string
  }

  export interface GrantRecord {
    type: string
    promotionId: string
  }

  export interface RewardsBalance {
    total: number
    rates: Record<string, number>
    wallets: Record<string, number>
  }

  // In-memory state is a superset of PersistentState
  export type State = PersistentState & EphemeralState
}
