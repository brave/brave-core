/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  NewTabPageAdEventType,
  NewTabPageAdMetricType,
} from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'

import {
  BraveBackground,
  SponsoredImageBackground,
  SelectedBackground,
  SelectedBackgroundType,
} from 'gen/brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.m.js'

export {
  BraveBackground,
  NewTabPageAdEventType,
  NewTabPageAdMetricType,
  SponsoredImageBackground,
  SelectedBackgroundType,
}

export type Background =
  | ({ type: 'brave' } & BraveBackground)
  | { type: 'color'; cssValue: string }
  | { type: 'custom'; imageUrl: string }
  | ({
      type: 'sponsored-image' | 'sponsored-rich-media'
    } & SponsoredImageBackground)

export interface BackgroundState {
  initialized: boolean
  backgroundsEnabled: boolean
  backgroundsCustomizable: boolean
  sponsoredImagesEnabled: boolean
  braveBackgrounds: BraveBackground[]
  customBackgrounds: string[]
  selectedBackground: SelectedBackground
  backgroundRotateIndex: number
  backgroundRandomValue: number
  sponsoredImageBackground: SponsoredImageBackground | null
  sponsoredRichMediaBaseUrl: string
}

export const solidBackgrounds = [
  '#5B5C63',
  '#000000',
  '#151E9A',
  '#2197F9',
  '#1FC3DC',
  '#086582',
  '#67D4B4',
  '#077D5A',
  '#3C790B',
  '#AFCE57',
  '#F0CB44',
  '#F28A29',
  '#FC798F',
  '#C1226E',
  '#FAB5EE',
  '#C0C4FF',
  '#9677EE',
  '#5433B0',
  '#4A000C',
]

export const solidPreviewBackground = solidBackgrounds[2]

export const gradientBackgrounds = [
  'linear-gradient(125.83deg, #392DD1 0%, #A91B78 99.09%)',
  'linear-gradient(125.83deg, #392DD1 0%, #22B8CF 99.09%)',
  'linear-gradient(90deg, #4F30AB 0.64%, #845EF7 99.36%)',
  'linear-gradient(126.47deg, #A43CE4 16.99%, #A72B6D 86.15%)',
  'radial-gradient('
    + '69.45% 69.45% at 89.46% 81.73%, #641E0C 0%, #500F39 43.54%, '
    + '#060141 100%)',
  'radial-gradient(80% 80% at 101.61% 76.99%, #2D0264 0%, #030023 100%)',
  'linear-gradient(128.12deg, #43D4D4 6.66%, #1596A9 83.35%)',
  'linear-gradient(323.02deg, #DD7131 18.65%, #FBD460 82.73%)',
  'linear-gradient(128.12deg, #4F86E2 6.66%, #694CD9 83.35%)',
  'linear-gradient(127.39deg, #851B6A 6.04%, #C83553 86.97%)',
  'linear-gradient(130.39deg, #FE6F4C 9.83%, #C53646 85.25%)',
]

export const gradientPreviewBackground = gradientBackgrounds[0]

export function defaultBackgroundState(): BackgroundState {
  return {
    initialized: false,
    backgroundsEnabled: true,
    backgroundsCustomizable: true,
    sponsoredImagesEnabled: true,
    braveBackgrounds: [],
    customBackgrounds: [],
    selectedBackground: {
      type: SelectedBackgroundType.kGradient,
      value: gradientPreviewBackground,
    },
    backgroundRotateIndex: 0,
    backgroundRandomValue: 0,
    sponsoredImageBackground: null,
    sponsoredRichMediaBaseUrl: '',
  }
}

export interface BackgroundActions {
  setBackgroundsEnabled: (enabled: boolean) => void
  setSponsoredImagesEnabled: (enabled: boolean) => void
  selectBackground: (type: SelectedBackgroundType, value: string) => void
  showCustomBackgroundChooser: () => Promise<boolean>
  removeCustomBackground: (background: string) => Promise<void>
  notifySponsoredImageLoadError: () => void
  notifySponsoredImageLogoClicked: () => void
  notifySponsoredRichMediaEvent: (type: NewTabPageAdEventType) => void
}

function chooseRandom<T>(list: T[], randomValue: number): T | null {
  if (list.length === 0) {
    return null
  }
  return list[Math.floor(randomValue * list.length)]
}

function chooseIndex<T>(list: T[], index: number): T | null {
  if (list.length === 0) {
    return null
  }
  return list[index % list.length]
}

const defaultBackground: Background = {
  type: 'color',
  cssValue: gradientPreviewBackground,
}

export function getCurrentBackground(
  state: BackgroundState,
): Background | null {
  const {
    initialized,
    backgroundsEnabled,
    braveBackgrounds,
    customBackgrounds,
    selectedBackground,
    backgroundRandomValue: randomValue,
    backgroundRotateIndex: rotateIndex,
    sponsoredImageBackground,
  } = state

  if (!initialized) {
    return null
  }

  if (!backgroundsEnabled) {
    return defaultBackground
  }

  if (sponsoredImageBackground) {
    return {
      type:
        sponsoredImageBackground.wallpaperType === 'richMedia'
          ? 'sponsored-rich-media'
          : 'sponsored-image',
      ...sponsoredImageBackground,
    }
  }

  const { type, value } = selectedBackground

  switch (type) {
    case SelectedBackgroundType.kBrave: {
      const braveBackground = chooseIndex(braveBackgrounds, rotateIndex)
      return braveBackground ? { type: 'brave', ...braveBackground } : null
    }
    case SelectedBackgroundType.kCustom: {
      const imageUrl = value || chooseIndex(customBackgrounds, rotateIndex)
      return imageUrl ? { type: 'custom', imageUrl } : null
    }
    case SelectedBackgroundType.kSolid: {
      const cssValue = value || chooseRandom(solidBackgrounds, randomValue)
      return cssValue ? { type: 'color', cssValue } : null
    }
    case SelectedBackgroundType.kGradient: {
      const cssValue = value || chooseRandom(gradientBackgrounds, randomValue)
      return cssValue ? { type: 'color', cssValue } : null
    }
    default: {
      console.error('Unhandled background type', type)
      return defaultBackground
    }
  }
}

export function backgroundCSSValue(
  type: SelectedBackgroundType,
  value: string,
) {
  switch (type) {
    case SelectedBackgroundType.kBrave:
    case SelectedBackgroundType.kCustom:
      return `url(${CSS.escape(value)})`
    default:
      return value
  }
}
