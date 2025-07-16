/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  BackgroundState,
  BackgroundActions,
  SponsoredImageBackground,
  SelectedBackgroundType,
  getCurrentBackground } from '../state/background_state'

function delay(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms)
  })
}

const sampleBackground =
    'https://brave.com/static-assets/images/brave-logo-sans-text.svg'

const sponsoredBackgrounds: Record<string, SponsoredImageBackground | null> = {
  image: {
    wallpaperType: '',
    imageUrl: sampleBackground,
    campaignId: '1234',
    creativeInstanceId: '',
    wallpaperId: '',
    logo: {
      alt: 'Be Brave!',
      destinationUrl: 'https://brave.com',
      imageUrl: sampleBackground
    },
    shouldMetricsFallbackToP3a: false
  },

  richMedia: {
    wallpaperType: 'richMedia',
    imageUrl: 'https://en.wikipedia.org/wiki/Main_Page',
    campaignId: '1234',
    creativeInstanceId: '',
    wallpaperId: '',
    logo: {
      alt: 'Be Brave!',
      destinationUrl: 'https://brave.com',
      imageUrl: ''
    },
    shouldMetricsFallbackToP3a: false
  },

  none: null
}

export function createBackgroundHandler(
  store: Store<BackgroundState>
): BackgroundActions {
  store.update({
    initialized: true,
    braveBackgrounds: [
      {
        author: 'John Doe',
        imageUrl: sampleBackground,
        link: 'https://brave.com'
      }
    ],
    sponsoredImageBackground: sponsoredBackgrounds.none
  })

  store.update({
    currentBackground: getCurrentBackground(store.getState()),
    sponsoredRichMediaBaseUrl: 'https://brave.com'
  })

  return {

    setBackgroundsEnabled(enabled) {
      store.update({ backgroundsEnabled: enabled })
      store.update({
        currentBackground: getCurrentBackground(store.getState())
      })
    },

    setSponsoredImagesEnabled(enabled) {
      store.update({ sponsoredImagesEnabled: enabled })
    },

    selectBackground(type, value) {
      store.update({
        selectedBackground: { type, value }
      })
      store.update({
        currentBackground: getCurrentBackground(store.getState())
      })
    },

    async showCustomBackgroundChooser() {
      delay(200).then(() => {
        store.update((state) => ({
          customBackgrounds: [...state.customBackgrounds, sampleBackground],
          selectedBackground: {
            type: SelectedBackgroundType.kCustom,
            value: sampleBackground
          }
        }))
        store.update({
          currentBackground: getCurrentBackground(store.getState())
        })
      })

      return true
    },

    async removeCustomBackground(background) {
      store.update((state) => ({
        customBackgrounds:
          state.customBackgrounds.filter((elem) => elem !== background)
      }))
    },

    notifySponsoredImageLoadError() {},

    notifySponsoredImageLogoClicked() {},

    notifySponsoredRichMediaEvent(type) {}
  }
}
