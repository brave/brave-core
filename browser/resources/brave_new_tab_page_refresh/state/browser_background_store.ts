/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import { SponsoredContentAdEventHandler } from 'gen/brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.m.js'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { debounce } from '$web-common/debounce'
import { preloadedBackgrounds } from './background_images/preloaded'
import { BackgroundActions, defaultBackgroundStore } from './background_store'

export function createBackgroundStore() {
  const store = defaultBackgroundStore()
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const sponsoredContentAdEventHandler =
    SponsoredContentAdEventHandler.getRemote()

  store.update({
    braveBackgrounds: preloadedBackgrounds,
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

  async function updateNewTabTakeoversEnabled() {
    const { enabled } = await handler.getNewTabTakeoversEnabled()
    store.update({ newTabTakeoversEnabled: enabled })
  }

  async function updateBraveBackgrounds() {
    const { backgrounds } = await handler.getBraveBackgrounds()
    store.update({
      braveBackgrounds: backgrounds.length ? backgrounds : preloadedBackgrounds,
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

  async function updateNewTabTakeoverBackground() {
    const { background } = await handler.getNewTabTakeoverBackground()
    store.update({ newTabTakeoverBackground: background ?? null })
  }

  newTabProxy.addListeners({
    onBackgroundsUpdated: debounce(async () => {
      await Promise.all([
        updateNewTabTakeoversEnabled(),
        updateCustomBackgrounds(),
        updateSelectedBackground(),
      ])
    }, 10),
  })

  async function loadData() {
    await Promise.all([
      updateBackgroundsEnabled(),
      updateNewTabTakeoversEnabled(),
      updateBraveBackgrounds(),
      updateCustomBackgrounds(),
      updateSelectedBackground(),
      updateNewTabTakeoverBackground(),
    ])

    store.update({ initialized: true })
  }

  loadData()

  const actions: BackgroundActions = {
    setBackgroundsEnabled(enabled) {
      store.update({ backgroundsEnabled: enabled })
      handler.setBackgroundsEnabled(enabled)
    },

    setNewTabTakeoversEnabled(enabled) {
      store.update({ newTabTakeoversEnabled: enabled })
      handler.setNewTabTakeoversEnabled(enabled)
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

    notifyNewTabTakeoverLoadError() {
      console.error('New Tab Takeover failed to load')
      store.update({ newTabTakeoverBackground: null })
    },

    notifyNewTabTakeoverLogoClicked() {
      const { newTabTakeoverBackground } = store.getState()
      if (newTabTakeoverBackground && newTabTakeoverBackground.logo) {
        handler.notifyNewTabTakeoverLogoClicked(
          newTabTakeoverBackground.wallpaperId,
          newTabTakeoverBackground.creativeInstanceId,
          newTabTakeoverBackground.logo.destinationUrl,
          newTabTakeoverBackground.metricType,
        )
      }
    },

    notifyNewTabTakeoverRichMediaEvent(type) {
      const { newTabTakeoverBackground } = store.getState()
      if (!newTabTakeoverBackground) {
        return
      }
      sponsoredContentAdEventHandler.maybeReportSponsoredContentAdEvent(
        newTabTakeoverBackground.wallpaperId,
        newTabTakeoverBackground.creativeInstanceId,
        newTabTakeoverBackground.metricType,
        type,
      )
    },
  }

  store.update({ actions })

  return store
}

function nextRotateIndex() {
  const rotateIndexKey = 'ntp-background-index'
  const index = parseInt(localStorage.getItem(rotateIndexKey) ?? '0') || 0
  localStorage.setItem(rotateIndexKey, String((index + 1) % 2 ** 32))
  return index
}
