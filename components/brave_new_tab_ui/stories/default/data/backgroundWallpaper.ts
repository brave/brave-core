// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CHANGE } from '@storybook/addon-knobs'
import { addons } from '@storybook/manager-api'

import { images, solidColorsForBackground, gradientColorsForBackground } from '../../../data/backgrounds'

const addonsChannel = addons.getChannel()

const generateWallpapers = function (images: NewTab.BackgroundWallpaper[],
  solidColors: NewTab.ColorBackground[],
  gradientColors: NewTab.ColorBackground[]) {
  let staticImages = { defaultImage: undefined }
  for (const image of images) {
    // author is optional field.
    if (image.type !== 'brave' || !image.author) {
      continue
    }

    Object.assign(staticImages, {
      [image.author]: {
        ...image,
        wallpaperImageUrl: require('../../../../img/newtab/backgrounds/' + image.wallpaperImageUrl)
      }
    })

    if (!staticImages.defaultImage) {
      staticImages.defaultImage = staticImages[image.author]
    }
  }

  const reducer = (prev: any, colorBackground: NewTab.ColorBackground) => {
    return {
      ...prev,
      [colorBackground.wallpaperColor]: colorBackground
    }
  }

  staticImages = solidColors.reduce(reducer, staticImages)
  staticImages = gradientColors.reduce(reducer, staticImages)

  return staticImages
}.bind(null, images, solidColorsForBackground, gradientColorsForBackground)

export const backgroundWallpapers = generateWallpapers()

/**
 * Mock handler for colored backgrounds. Emits a change event to knobs
 * @param {string} value
 */
export const onChangeColoredBackground = (value: string, useRandomValue: boolean) => {
  addonsChannel.emit(CHANGE, {
    name: 'Show branded background image?',
    value: false
  })
  addonsChannel.emit(CHANGE, {
    name: 'Background',
    value: backgroundWallpapers[value]
  })
}

/**
 * Mock handler for Brave background. Emits a change event to knobs.
 * @param {string} selectedBackground - selected background URL
 * in storybook, we have only one preset image.
 */
export const onUseBraveBackground = (selectedBackground: string) => {
  addonsChannel.emit(CHANGE, {
    name: 'Background',
    value: backgroundWallpapers.defaultImage
  })
}

export const onShowBrandedImageChanged = (show: boolean) => {
  addonsChannel.emit(CHANGE, {
    name: 'Show branded background image?',
    value: show
  })
}
