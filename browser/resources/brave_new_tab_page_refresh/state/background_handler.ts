/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { SponsoredRichMediaAdEventHandler } from 'gen/brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.m.js'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounce } from '$web-common/debounce'
import {
  BackgroundState,
  BackgroundActions,
  BraveBackground,
} from './background_state'

// A pre-loaded background image resource that can be used if a new profile
// opens the NTP before the NTPBackgroundImagesService has finished loading
// the current collection of Brave backgrounds.
const defaultBraveBackground: BraveBackground = {
  imageUrl: 'dylan-malval_sea-min.webp',
  author: 'Dylan Malval',
  link: 'https://www.instagram.com/vass_captures/',
}

export function createBackgroundHandler(
  store: Store<BackgroundState>,
): BackgroundActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const sponsoredRichMediaAdEventHandler =
    SponsoredRichMediaAdEventHandler.getRemote()

  store.update({
    braveBackgrounds: [defaultBraveBackground],
    backgroundRandomValue: Math.random(),
    backgroundRotateIndex: nextRotateIndex(),
    backgroundsCustomizable: loadTimeData.getBoolean(
      'customBackgroundFeatureEnabled',
    ),
    sponsoredRichMediaBaseUrl: loadTimeData.getString(
      'sponsoredRichMediaBaseUrl',
    ),
  })

  async function updateBackgroundsEnabled() {
    const { enabled } = await handler.getBackgroundsEnabled()
    store.update({ backgroundsEnabled: enabled })
  }

  async function updateSponsoredImagesEnabled() {
    const { enabled } = await handler.getSponsoredImagesEnabled()
    store.update({ sponsoredImagesEnabled: enabled })
  }

  async function updateBraveBackgrounds() {
    const { backgrounds } = await handler.getBraveBackgrounds()
    store.update({
      braveBackgrounds: backgrounds.length
        ? backgrounds
        : [defaultBraveBackground],
    })
  }

  async function updateSelectedBackground() {
    const { background } = await handler.getSelectedBackground()
    if (background) {
      store.update({ selectedBackground: background })
    }
  }

  async function updateCustomBackgrounds() {
    const { backgrounds } = await handler.getCustomBackgrounds()
    store.update({ customBackgrounds: backgrounds })
  }

  async function updateSponsoredImageBackground() {
    const { background } = await handler.getSponsoredImageBackground()
    store.update({ sponsoredImageBackground: background ?? null })
  }

  newTabProxy.addListeners({
    onBackgroundsUpdated: debounce(async () => {
      await Promise.all([updateCustomBackgrounds(), updateSelectedBackground()])
    }, 10),
  })

  async function loadData() {
    await Promise.all([
      updateBackgroundsEnabled(),
      updateSponsoredImagesEnabled(),
      updateBraveBackgrounds(),
      updateCustomBackgrounds(),
      updateSelectedBackground(),
      updateSponsoredImageBackground(),
    ])

    store.update({ initialized: true })
  }

  loadData()

  return {
    setBackgroundsEnabled(enabled) {
      store.update({ backgroundsEnabled: enabled })
      handler.setBackgroundsEnabled(enabled)
    },

    setSponsoredImagesEnabled(enabled) {
      store.update({ sponsoredImagesEnabled: enabled })
      handler.setSponsoredImagesEnabled(enabled)
    },

    selectBackground(type, value) {
      store.update({ selectedBackground: { type, value } })
      if (!value) {
        store.update({ backgroundRandomValue: Math.random() })
      }
      handler.selectBackground({ type, value })
    },

    async showCustomBackgroundChooser() {
      const { imagesSelected } = await handler.showCustomBackgroundChooser()
      return imagesSelected
    },

    async removeCustomBackground(background) {
      await handler.removeCustomBackground(background)
    },

    notifySponsoredImageLoadError() {
      console.error('Sponsored image failed to load')
      store.update({ sponsoredImageBackground: null })
    },

    notifySponsoredImageLogoClicked() {
      const { sponsoredImageBackground } = store.getState()
      if (sponsoredImageBackground && sponsoredImageBackground.logo) {
        handler.notifySponsoredImageLogoClicked(
          sponsoredImageBackground.wallpaperId,
          sponsoredImageBackground.creativeInstanceId,
          sponsoredImageBackground.logo.destinationUrl,
          sponsoredImageBackground.metricType,
        )
      }
    },

    notifySponsoredRichMediaEvent(type) {
      const { sponsoredImageBackground } = store.getState()
      if (!sponsoredImageBackground) {
        return
      }
      sponsoredRichMediaAdEventHandler.maybeReportRichMediaAdEvent(
        sponsoredImageBackground.wallpaperId,
        sponsoredImageBackground.creativeInstanceId,
        sponsoredImageBackground.metricType,
        type,
      )
    },
  }
}

function nextRotateIndex() {
  const rotateIndexKey = 'ntp-background-index'
  const index = parseInt(localStorage.getItem(rotateIndexKey) ?? '0') || 0
  localStorage.setItem(rotateIndexKey, String((index + 1) % 2 ** 32))
  return index
}
