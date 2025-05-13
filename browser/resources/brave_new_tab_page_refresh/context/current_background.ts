/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  BraveBackground,
  SponsoredImageBackground,
  SelectedBackgroundType,
  gradientPreviewBackground,
  gradientBackgrounds,
  solidBackgrounds,
  useBackgroundState } from './backgrounds'

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

function createRandomizer(random: () => number) {
  let value = random()
  return {
    select<T>(list: T[]) {
      return list.length > 0 ? list[Math.floor(value * list.length)] : null
    },
    reset() {
      value = random()
    }
  }
}

const context = React.createContext<CurrentBackground>(defaultBackground)

interface Props {
  getRandomValue: () => number
  children: React.ReactNode
}

export function CurrentBackgroundProvider(props: Props) {
  const backgroundsEnabled = useBackgroundState((s) => s.backgroundsEnabled)
  const braveBackgrounds = useBackgroundState((s) => s.braveBackgrounds)
  const customBackgrounds = useBackgroundState((s) => s.customBackgrounds)
  const selectedBackground = useBackgroundState((s) => s.selectedBackground)
  const sponsoredImageBackground =
    useBackgroundState((s) => s.sponsoredImageBackground)

  const randomizerRef = React.useRef(createRandomizer(props.getRandomValue))

  const background: CurrentBackground | null = React.useMemo(() => {
    if (sponsoredImageBackground) {
      return {
        type: sponsoredImageBackground.wallpaperType === 'richMedia' ?
          'sponsored-rich-media' : 'sponsored-image',
        ...sponsoredImageBackground
      }
    }

    const { type, value } = selectedBackground
    const randomizer = randomizerRef.current

    switch (type) {
      case SelectedBackgroundType.kBrave: {
        const braveBackground = randomizer.select(braveBackgrounds)
        return braveBackground ? { type: 'brave', ...braveBackground } : null
      }
      case SelectedBackgroundType.kCustom: {
        const imageUrl = value || randomizer.select(customBackgrounds)
        return imageUrl ? { type: 'custom', imageUrl } : null
      }
      case SelectedBackgroundType.kSolid: {
        const cssValue = value || randomizer.select(solidBackgrounds)
        return cssValue ? { type: 'color', cssValue } : null
      }
      case SelectedBackgroundType.kGradient: {
        const cssValue = value || randomizer.select(gradientBackgrounds)
        return cssValue ? { type: 'color', cssValue } : null
      }
      default: {
        console.error('Unhandled background type', type)
        return null
      }
    }
  }, [
    braveBackgrounds,
    customBackgrounds,
    selectedBackground,
    sponsoredImageBackground
  ])

  const currentBackground =
    backgroundsEnabled && background ? background : defaultBackground

  return React.createElement(
    context.Provider, { value: currentBackground }, props.children)
}

export function useCurrentBackground() {
  return React.useContext(context)
}
