/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  BackgroundAPI,
  BraveBackground,
  SponsoredImageBackground,
  SelectedBackgroundType,
  gradientPreviewBackground,
  gradientBackgrounds,
  solidBackgrounds } from '../api/background_api'

import { createAPIProvider } from '../lib/api_provider'

export const {
  Provider: BackgroundProvider,
  useState: useBackgroundState,
  useActions: useBackgroundActions } =
    createAPIProvider(React.createContext<BackgroundAPI | null>(null))

export type CurrentBackground =
  { type: 'brave' } & BraveBackground |
  { type: 'color', cssValue: string } |
  { type: 'custom', imageUrl: string } |
  { type: 'sponsored-image' | 'sponsored-rich-media' }
    & SponsoredImageBackground

const defaultBackground: CurrentBackground = {
  type: 'color',
  cssValue: gradientPreviewBackground
}

export function useCurrentBackground() {
  const backgroundsEnabled = useBackgroundState((s) => s.backgroundsEnabled)
  const backgroundRandomizer = useBackgroundState((s) => s.backgroundRandomizer)
  const braveBackgrounds = useBackgroundState((s) => s.braveBackgrounds)
  const customBackgrounds = useBackgroundState((s) => s.customBackgrounds)
  const selectedBackground = useBackgroundState((s) => s.selectedBackground)
  const sponsoredImageBackground =
    useBackgroundState((s) => s.sponsoredImageBackground)

  function chooseRandom<T>(list: T[]) {
    const randomIndex = Math.floor(backgroundRandomizer * list.length)
    return list.length > 0 ? list[randomIndex] : null
  }

  const currentBackground: CurrentBackground | null = React.useMemo(() => {
    if (!backgroundsEnabled) {
      return null
    }

    if (sponsoredImageBackground) {
      return {
        type: sponsoredImageBackground.wallpaperType === 'richMedia' ?
          'sponsored-rich-media' : 'sponsored-image',
        ...sponsoredImageBackground
      }
    }

    const { type, value } = selectedBackground

    switch (type) {
      case SelectedBackgroundType.kBrave: {
        const braveBackground = chooseRandom(braveBackgrounds)
        return braveBackground ? { type: 'brave', ...braveBackground } : null
      }
      case SelectedBackgroundType.kCustom: {
        const imageUrl = value || chooseRandom(customBackgrounds)
        return imageUrl ? { type: 'custom', imageUrl } : null
      }
      case SelectedBackgroundType.kSolid: {
        const cssValue = value || chooseRandom(solidBackgrounds)
        return cssValue ? { type: 'color', cssValue } : null
      }
      case SelectedBackgroundType.kGradient: {
        const cssValue = value || chooseRandom(gradientBackgrounds)
        return cssValue ? { type: 'color', cssValue } : null
      }
      default: {
        console.error('Unhandled background type', type)
        return null
      }
    }
  }, [
    backgroundRandomizer,
    braveBackgrounds,
    customBackgrounds,
    selectedBackground.type,
    selectedBackground.value,
    sponsoredImageBackground
  ])

  return currentBackground ?? defaultBackground
}
