/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'
import { getCurrentBackground } from '../models/backgrounds'

import {
  NewTabModel,
  SponsoredImageBackground,
  defaultModel,
  defaultState } from '../models/new_tab_model'

function delay(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms)
  })
}

const sampleBackground =
  'https://brave.com/static-assets/images/brave-logo-sans-text.svg'

const sampleSponsoredImage: SponsoredImageBackground = {
  type: 'sponsored',
  imageUrl: sampleBackground,
  creativeInstanceId: '',
  wallpaperId: '',
  logo: {
    alt: 'Be Brave!',
    destinationUrl: 'https://brave.com',
    imageUrl: sampleBackground
  }
}

export function createNewTabModel(): NewTabModel {
  const store = createStore(defaultState())
  store.update({
    braveBackgrounds: [
      {
        type: 'brave',
        author: 'John Doe',
        imageUrl: sampleBackground,
        link: 'https://brave.com'
      }
    ],
    sponsoredImageBackground: sampleSponsoredImage && null,
    showClock: true
  })

  store.update({
    currentBackground: getCurrentBackground(store.getState())
  })

  return {
    ...defaultModel(),

    getState: store.getState,
    addListener: store.addListener,

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
    },

    setClockFormat(format) {
      store.update({ clockFormat: format })
    },

    setShowClock(showClock) {
      store.update({ showClock })
    }
  }
}
