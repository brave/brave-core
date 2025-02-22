/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export type BackgroundType = 'brave' | 'custom' | 'solid' | 'gradient' | 'none'

export interface BraveBackground {
  type: 'brave'
  author: string
  imageUrl: string
  link: string
}

export interface ColorBackground {
  type: 'solid' | 'gradient'
  cssValue: string
}

export interface CustomBackground {
  type: 'custom'
  imageUrl: string
}

export interface SponsoredImageLogo {
  alt: string
  destinationUrl: string
  imageUrl: string
}

export interface SponsoredImageBackground {
  type: 'sponsored-image' | 'sponsored-rich-media'
  imageUrl: string
  creativeInstanceId: string
  campaignId: string
  wallpaperId: string
  logo: SponsoredImageLogo | null
}

export type SponsoredRichMediaEventType =
  'click' |
  'interaction' |
  'mediaPlay' |
  'media25' |
  'media100'

export type Background =
  BraveBackground |
  ColorBackground |
  CustomBackground |
  SponsoredImageBackground

export type ClockFormat = '' | 'h12' | 'h24'

export interface ShieldsStats {
  bandwidthSavedBytes: number
  adsBlocked: number
}

export type WidgetPosition = 'top' | 'bottom'

export interface NewTabState {
  backgroundsEnabled: boolean
  backgroundsCustomizable: boolean
  sponsoredImagesEnabled: boolean
  braveBackgrounds: BraveBackground[]
  customBackgrounds: string[]
  selectedBackgroundType: BackgroundType
  selectedBackground: string
  currentBackground: Background | null
  sponsoredImageBackground: SponsoredImageBackground | null
  sponsoredRichMediaBaseUrl: string
  showClock: boolean
  clockFormat: ClockFormat
  showShieldsStats: boolean
  shieldsStats: ShieldsStats | null
  showTalkWidget: boolean
  widgetPosition: WidgetPosition
}

export function defaultState(): NewTabState {
  return {
    backgroundsEnabled: true,
    backgroundsCustomizable: true,
    sponsoredImagesEnabled: true,
    braveBackgrounds: [],
    customBackgrounds: [],
    selectedBackgroundType: 'none',
    selectedBackground: '',
    currentBackground: null,
    sponsoredImageBackground: null,
    sponsoredRichMediaBaseUrl: '',
    showClock: false,
    clockFormat: '',
    showShieldsStats: false,
    shieldsStats: null,
    showTalkWidget: true,
    widgetPosition: 'bottom'
  }
}

export interface NewTabModel {
  getState: () => NewTabState
  addListener: (listener: (state: NewTabState) => void) => () => void
  getPcdnImageURL: (url: string) => Promise<string>
  setBackgroundsEnabled: (enabled: boolean) => void
  setSponsoredImagesEnabled: (enabled: boolean) => void
  selectBackground: (type: BackgroundType, value: string) => void
  showCustomBackgroundChooser: () => Promise<boolean>
  removeCustomBackground: (background: string) => Promise<void>
  notifySponsoredImageLogoClicked: () => void
  notifySponsoredRichMediaEvent: (type: SponsoredRichMediaEventType) => void
  setShowClock: (showClock: boolean) => void
  setClockFormat: (format: ClockFormat) => void
  setShowShieldsStats: (showShieldsStats: boolean) => void
  setShowTalkWidget: (showTalkWidget: boolean) => void
  setWidgetPosition: (widgetPosition: WidgetPosition) => void
}

export function defaultModel(): NewTabModel {
  const state = defaultState()
  return {
    getState() { return state },
    addListener() { return () => {} },
    async  getPcdnImageURL(url) { return url },
    setBackgroundsEnabled(enabled) {},
    setSponsoredImagesEnabled(enabled) {},
    selectBackground(type, value) {},
    async showCustomBackgroundChooser() { return false },
    async removeCustomBackground(background) {},
    notifySponsoredImageLogoClicked() {},
    notifySponsoredRichMediaEvent(type) {},
    setShowClock(showClock) {},
    setClockFormat(format) {},
    setShowShieldsStats(showShieldsStats) {},
    setShowTalkWidget(showTalkWidget) {},
    setWidgetPosition(widgetPosition: WidgetPosition) {}
  }
}
