// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

declare namespace NewTab {

  export type BrandedWallpaperLogo = {
    image: string
    companyName: string
    alt: string
    destinationUrl: string
  }

  export interface BrandedWallpaper {
    wallpaperImageUrl: string
    logo: BrandedWallpaperLogo
  }
  export interface ApplicationState {
    newTabData: State | undefined
    gridSitesData: GridSitesState | undefined
  }

  export interface Image {
    name: string
    source: string
    author: string
    link: string
    originalUrl: string
    license: string
  }

  export interface Site {
    id: string
    url: string
    title: string
    favicon: string
    letter: string
    pinnedIndex: number | undefined
    bookmarkInfo: chrome.bookmarks.BookmarkTreeNode | undefined
  }

  // This is preserved for migration reasons.
  // Do not tyoe new code using this interface.
  export interface LegacySite {
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

  export type StackWidget = 'rewards' | 'binance'

  export interface LegacyState {
    pinnedTopSites: Site[]
    ignoredTopSites: Site[]
  }

  export interface GridSitesState {
    removedSites: Site[]
    gridSites: Site[]
    shouldShowSiteRemovedNotification: boolean
    legacy: LegacyState
  }

  export interface PageState {
    showEmptyPage: boolean
  }

  export interface RewardsState {
    rewardsState: RewardsWidgetState
    currentStackWidget: StackWidget
  }

  export interface PersistentState {
    showEmptyPage: boolean
    rewardsState: RewardsWidgetState
    currentStackWidget: StackWidget
    binanceState: BinanceWidgetState
  }

  export interface EphemeralState {
    initialDataLoaded: boolean
    textDirection: string
    featureFlagBraveNTPBrandedWallpaper: boolean
    isIncognito: boolean
    useAlternativePrivateSearchEngine: boolean
    isTor: boolean
    isQwant: boolean
    backgroundImage?: Image
    gridLayoutSize?: 'small'
    showGridSiteRemovedNotification?: boolean
    showBackgroundImage: boolean
    showStats: boolean
    showClock: boolean
    showTopSites: boolean
    showRewards: boolean
    showBinance: boolean
    brandedWallpaperOptIn: boolean
    isBrandedWallpaperNotificationDismissed: boolean
    stats: Stats,
    brandedWallpaperData?: BrandedWallpaper
  }

  export interface RewardsWidgetState {
    adsEstimatedEarnings: number
    adsSupported?: boolean
    balance: RewardsBalance
    dismissedNotifications: string[]
    enabledAds: boolean
    enabledMain: boolean
    promotions: Promotion[]
    onlyAnonWallet: boolean
    totalContribution: number
    walletCreated: boolean
    walletCreating: boolean
    walletCreateFailed: boolean
    walletCorrupted: boolean
  }

  export interface BinanceWidgetState {
    userTLD: BinanceTLD
    initialFiat: string
    initialAmount: string
    initialAsset: string
    userTLDAutoSet: boolean
    binanceSupported: boolean
    accountBalances: Record<string, string>
    authInProgress: boolean
    assetBTCValues: Record<string, string>
    assetUSDValues: Record<string, string>
    assetBTCVolumes: Record<string, string>
    userAuthed: boolean
    btcBalanceValue: string
    hideBalance: boolean
    btcPrice: string
    btcVolume: string
    binanceClientUrl: string
    assetDepositInfo: Record<string, any>
    assetDepoitQRCodeSrcs: Record<string, string>
  }

  export type BinanceTLD = 'us' | 'com'

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
    WALLET_CORRUPT = 17
  }

  export interface RewardsBalanceReport {
    ads: number
    contribute: number
    monthly: number
    grant: number
    tips: number
  }

  export enum PromotionTypes {
    UGP = 0,
    ADS = 1
  }

  export interface PromotionResponse {
    result: number
    promotions: Promotion[]
  }

  export interface Promotion {
    type: PromotionTypes
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
