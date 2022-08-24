// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { CHANGE } from '@storybook/addon-knobs'
import { addons } from '@storybook/addons'

import { images } from '../../../data/backgrounds'
import { solidColorsForBackground, gradientColorsForBackground } from '../../../data/colors'

const addonsChannel = addons.getChannel()

export const backgroundWallpapers = (function (images: NewTab.BackgroundWallpaper[],
                                               solidColors: string[],
                                               gradientColors: string[]) {
  let staticImages = { defaultImage: undefined }
  for (const image of images) {
    // author is optional field.
    if (image.type !== 'image' || !image.author) {
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

  for (const color of solidColors) {
    Object.assign(staticImages, {
      [color]: {
        type: 'solidColor',
        wallpaperSolidColor: color
      }
    })
  }

  staticImages = gradientColors.reduce((prev, gradient) => {
    return {
      ...prev,
      [gradient]: {
        type: 'gradientColor',
        wallpaperGradientColor: gradient
      }
    }
  }, staticImages)

  return staticImages
})(images, solidColorsForBackground, gradientColorsForBackground)

/**
 * Mock handler for colored backgrounds. Emits a change event to knobs
 * @param {string} value
 */
export const onChangeColoredBackground = (value: string) => {
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
 * @param {boolean} useCustom - When it's false, uses images provide by Brave
 * in storybook, we have only one preset image.
 */
export const onUseCustomBackground = (useCustom: boolean) => {
  if (useCustom) {
    return
  }

  addonsChannel.emit(CHANGE, {
    name: 'Background',
    value: backgroundWallpapers.defaultImage
  })
}
