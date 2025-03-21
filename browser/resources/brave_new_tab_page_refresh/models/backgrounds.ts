/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export type BackgroundType = 'brave' | 'custom' | 'solid' | 'gradient'

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
  shouldMetricsFallbackToP3a: boolean
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

export interface BackgroundState {
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
}

export const solidBackgrounds = [
  '#5B5C63', '#000000', '#151E9A', '#2197F9', '#1FC3DC', '#086582', '#67D4B4',
  '#077D5A', '#3C790B', '#AFCE57', '#F0CB44', '#F28A29', '#FC798F', '#C1226E',
  '#FAB5EE', '#C0C4FF', '#9677EE', '#5433B0', '#4A000C'
]

export const solidPreviewBackground = solidBackgrounds[2]

export const gradientBackgrounds = [
  'linear-gradient(125.83deg, #392DD1 0%, #A91B78 99.09%)',
  'linear-gradient(125.83deg, #392DD1 0%, #22B8CF 99.09%)',
  'linear-gradient(90deg, #4F30AB 0.64%, #845EF7 99.36%)',
  'linear-gradient(126.47deg, #A43CE4 16.99%, #A72B6D 86.15%)',
  'radial-gradient(' +
    '69.45% 69.45% at 89.46% 81.73%, #641E0C 0%, #500F39 43.54%, #060141 100%)',
  'radial-gradient(80% 80% at 101.61% 76.99%, #2D0264 0%, #030023 100%)',
  'linear-gradient(128.12deg, #43D4D4 6.66%, #1596A9 83.35%)',
  'linear-gradient(323.02deg, #DD7131 18.65%, #FBD460 82.73%)',
  'linear-gradient(128.12deg, #4F86E2 6.66%, #694CD9 83.35%)',
  'linear-gradient(127.39deg, #851B6A 6.04%, #C83553 86.97%)',
  'linear-gradient(130.39deg, #FE6F4C 9.83%, #C53646 85.25%)'
]

export const gradientPreviewBackground = gradientBackgrounds[0]

const defaultBackgroundType: BackgroundType = 'gradient'

const defaultBackground: ColorBackground = {
  type: defaultBackgroundType,
  cssValue: gradientPreviewBackground
}

export function defaultBackgroundState(): BackgroundState {
  return {
    backgroundsEnabled: true,
    backgroundsCustomizable: true,
    sponsoredImagesEnabled: true,
    braveBackgrounds: [],
    customBackgrounds: [],
    selectedBackgroundType: defaultBackgroundType,
    selectedBackground: defaultBackground.cssValue,
    currentBackground: null,
    sponsoredImageBackground: null,
    sponsoredRichMediaBaseUrl: ''
  }
}

export interface BackgroundActions {
  setBackgroundsEnabled: (enabled: boolean) => void
  setSponsoredImagesEnabled: (enabled: boolean) => void
  selectBackground: (type: BackgroundType, value: string) => void
  showCustomBackgroundChooser: () => Promise<boolean>
  removeCustomBackground: (background: string) => Promise<void>
  notifySponsoredImageLogoClicked: () => void
  notifySponsoredRichMediaEvent: (type: SponsoredRichMediaEventType) => void
}

function chooseRandom<T>(list: T[]): T | null {
  if (list.length === 0) {
    return null
  }
  return list[Math.floor(Math.random() * list.length)]
}

export function getCurrentBackground(
  state: BackgroundState
) : Background | null {
  const {
    backgroundsEnabled,
    braveBackgrounds,
    customBackgrounds,
    selectedBackground,
    selectedBackgroundType,
    sponsoredImageBackground,
    currentBackground } = state

  if (!backgroundsEnabled) {
    return defaultBackground
  }

  if (sponsoredImageBackground) {
    return sponsoredImageBackground
  }

  if (currentBackground &&
      selectedBackgroundType === currentBackground.type &&
      !selectedBackground) {
    return currentBackground
  }

  switch (selectedBackgroundType) {
    case 'brave': {
      return chooseRandom(braveBackgrounds)
    }
    case 'custom': {
      const imageUrl = selectedBackground || chooseRandom(customBackgrounds)
      return imageUrl ? { type: 'custom', imageUrl } : null
    }
    case 'solid': {
      const cssValue = selectedBackground || chooseRandom(solidBackgrounds)
      return cssValue ? { type: 'solid', cssValue } : null
    }
    case 'gradient': {
      const cssValue = selectedBackground || chooseRandom(gradientBackgrounds)
      return cssValue ? { type: 'gradient', cssValue } : null
    }
  }
}

export function backgroundCSSValue(type: BackgroundType, value: string) {
  switch (type) {
    case 'brave':
    case 'custom': return `url(${CSS.escape(value)})`
    case 'solid':
    case 'gradient': return value
  }
}
