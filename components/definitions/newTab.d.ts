// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace NewTab {
  // Custom background with solid color or gradation
  export type ColorBackground = {
    type: 'color'
    wallpaperColor: string
    random?: boolean
  }

  // Backgrounds based on image. Custom image background or Brave background.
  export type ImageBackground = {
    type: 'image'
    wallpaperImageUrl: string
    random?: boolean
  }

  export type BraveBackground = Omit<ImageBackground, 'type'> & {
    type: 'brave'
    author: string
    link?: string
    originalUrl?: string
    license?: string

    // Picked by our rotating algorithm. When it's false, it means that the user
    // has picked this background.
    random?: boolean
  }

  export type BackgroundWallpaper = ColorBackground | ImageBackground | BraveBackground

  export type BrandedWallpaperLogo = {
    image: string
    companyName: string
    alt: string
    destinationUrl: string
  }

  export type BrandedWallpaper = {
    wallpaperImageUrl: string
    isSponsored: boolean
    creativeInstanceId: string
    wallpaperId: string
    logo: BrandedWallpaperLogo
  }

  export interface Wallpaper {
    backgroundWallpaper: BackgroundWallpaper
    brandedWallpaper?: BrandedWallpaper
  }

  export interface ApplicationState {
    newTabData: State | undefined
    gridSitesData: GridSitesState | undefined
  }

  export interface Site {
    id: string
    url: string
    title: string
    favicon: string
    letter: string
    pinnedIndex: number | undefined
    defaultSRTopSite: boolean | undefined
  }

  export interface Stats {
    adsBlockedStat: number
    httpsUpgradesStat: number
    javascriptBlockedStat: number
    bandwidthSavedStat: number
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

  export type StackWidget = 'rewards' | 'braveTalk' | 'braveVPN' | 'bitcoinDotCom' | ''

  export interface GridSitesState {
    removedSites: Site[]
    gridSites: Site[]
    shouldShowSiteRemovedNotification: boolean
  }

  export interface PageState {
    showEmptyPage: boolean
  }

  export interface RewardsState {
    rewardsState: RewardsWidgetState
  }

  export interface PersistentState {
    braveRewardsSupported: boolean
    braveTalkSupported: boolean
    bitcoinDotComSupported: boolean
    showEmptyPage: boolean
    rewardsState: RewardsWidgetState
    currentStackWidget: StackWidget
    removedStackWidgets: StackWidget[]
    widgetStackOrder: StackWidget[]
  }

  export type Preferences = {
    showBackgroundImage: boolean
    brandedWallpaperOptIn: boolean
    showStats: boolean
    showToday: boolean
    showClock: boolean
    clockFormat: string
    showTopSites: boolean
    showRewards: boolean
    showBraveTalk: boolean
    showBraveVPN: boolean
    showSearchBox: boolean
    lastUsedNtpSearchEngine: string
    promptEnableSearchSuggestions: boolean
    searchSuggestionsEnabled: boolean
    hideAllWidgets: boolean
    isBraveNewsOptedIn: boolean
    isBrandedWallpaperNotificationDismissed: boolean
  }

  export type EphemeralState = Preferences & {
    initialDataLoaded: boolean
    textDirection: string
    featureFlagBraveNTPSponsoredImagesWallpaper: boolean
    featureFlagBraveNewsPromptEnabled: boolean
    featureFlagBraveNewsFeedV2Enabled: boolean
    searchPromotionEnabled: boolean
    featureCustomBackgroundEnabled: boolean
    isIncognito: boolean
    torCircuitEstablished: boolean,
    torInitProgress: string,
    isTor: boolean
    gridLayoutSize?: 'small'
    showGridSiteRemovedNotification?: boolean
    showBackgroundImage: boolean
    customLinksEnabled: boolean
    customLinksNum: number
    showBitcoinDotCom: boolean
    stats: Stats,
    brandedWallpaper?: BrandedWallpaper
    backgroundWallpaper?: BackgroundWallpaper
    customImageBackgrounds: ImageBackground[]
  }

  export interface RewardsWidgetState {
    rewardsEnabled: boolean
    userType: string
    declaredCountry: string
    balance?: number
    externalWallet?: RewardsExtension.ExternalWallet
    externalWalletProviders?: string[]
    report?: RewardsBalanceReport
    adsAccountStatement: AdsAccountStatement
    dismissedNotifications: string[]
    needsBrowserUpgradeToServeAds: boolean
    parameters: RewardsParameters
    totalContribution: number
    publishersVisitedCount: number
    selfCustodyInviteDismissed: boolean
    isTermsOfServiceUpdateRequired: boolean
  }

  export const enum RewardsResult {
    OK = 0,
    FAILED = 1,
    NO_PUBLISHER_STATE = 2,
    NO_LEGACY_STATE = 3,
    INVALID_PUBLISHER_STATE = 4,
    CAPTCHA_FAILED = 6,
    NO_PUBLISHER_LIST = 7,
    TOO_MANY_RESULTS = 8,
    NOT_FOUND = 9,
    REGISTRATION_VERIFICATION_FAILED = 10,
    BAD_REGISTRATION_RESPONSE = 11,
    WALLET_CORRUPT = 17
  }

  export interface RewardsBalanceReport {
    ads: number
    contribute: number
    monthly: number
    tips: number
  }

  export interface AdsAccountStatement {
    nextPaymentDate: number
    adsReceivedThisMonth: number
    minEarningsThisMonth: number
    maxEarningsThisMonth: number
    minEarningsLastMonth: number
    maxEarningsLastMonth: number
  }

  export type ProviderPayoutStatus = 'off' | 'processing' | 'complete'

  export interface RewardsParameters {
    rate: number
    monthlyTipChoices: number[]
    payoutStatus?: Record<string, ProviderPayoutStatus>
    walletProviderRegions?: Record<string, { allow: string[], block: string[] } | undefined>
    vbatDeadline: number | undefined
    vbatExpired: boolean
  }

  export interface DefaultSuperReferralTopSite {
    pinnedIndex: number
    url: string
    title: string
    favicon: string
  }

  interface StorybookStateExtras {
    forceSettingsTab?: string // SettingsTabType
    readabilityThreshold?: number
  }

  // In-memory state is a superset of PersistentState
  export type State = PersistentState & EphemeralState & StorybookStateExtras
}
