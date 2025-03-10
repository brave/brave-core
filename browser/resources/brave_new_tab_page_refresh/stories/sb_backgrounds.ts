/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  BackgroundActions,
  BackgroundState,
  SponsoredImageBackground,
  defaultBackgroundActions,
  getCurrentBackground } from '../models/backgrounds'

function delay(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms)
  })
}

const sampleBackground =
  'https://brave.com/static-assets/images/brave-logo-sans-text.svg'

const sponsoredBackgrounds: Record<string, SponsoredImageBackground | null> = {
  image: {
    type: 'sponsored-image',
    imageUrl: sampleBackground,
    campaignId: '1234',
    creativeInstanceId: '',
    wallpaperId: '',
    logo: {
      alt: 'Be Brave!',
      destinationUrl: 'https://brave.com',
      imageUrl: sampleBackground
    }
  },

  richMedia: {
    type: 'sponsored-rich-media',
    imageUrl: 'https://en.wikipedia.org/wiki/Main_Page',
    campaignId: '1234',
    creativeInstanceId: '',
    wallpaperId: '',
    logo: {
      alt: 'Be Brave!',
      destinationUrl: 'https://brave.com',
      imageUrl: ''
    }
  },

  none: null
}

export function initializeBackgrounds(
    store: Store<BackgroundState>): BackgroundActions {
  store.update({
    braveBackgrounds: [
      {
        type: 'brave',
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

    ...defaultBackgroundActions(),

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
        selectedBackgroundType: type,
        selectedBackground: value
      })
      store.update({
        currentBackground: getCurrentBackground(store.getState())
      })
    },

    async showCustomBackgroundChooser() {
      delay(200).then(() => {
        store.update((state) => ({
          customBackgrounds: [...state.customBackgrounds, sampleBackground],
          selectedBackground: sampleBackground,
          selectedBackgroundType: 'custom'
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
    }
  }
}
