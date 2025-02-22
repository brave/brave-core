/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { SelectedBackgroundType } from 'gen/brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.m.js'
import { SponsoredRichMediaAdEventHandler } from 'gen/brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.m.js'
import { NewTabPageAdEventType } from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'
import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import { NewTabPageProxy } from './new_tab_page_proxy'
import { createStore } from '../lib/store'
import { getCurrentBackground } from '../models/backgrounds'
import { debounceListener } from './debounce_listener'

import {
  NewTabModel,
  WidgetPosition,
  SponsoredImageBackground,
  SponsoredRichMediaEventType,
  BackgroundType,
  defaultState } from '../models/new_tab_model'

function backgroundTypeFromMojo(type: number): BackgroundType {
  switch (type) {
    case SelectedBackgroundType.kBrave: return 'brave'
    case SelectedBackgroundType.kCustom: return 'custom'
    case SelectedBackgroundType.kSolid: return 'solid'
    case SelectedBackgroundType.kGradient: return 'gradient'
    default: return 'none'
  }
}

function backgroundTypeToMojo(type: BackgroundType) {
  switch (type) {
    case 'brave': return SelectedBackgroundType.kBrave
    case 'custom': return SelectedBackgroundType.kCustom
    case 'solid': return SelectedBackgroundType.kSolid
    case 'gradient': return SelectedBackgroundType.kGradient
    case 'none': return SelectedBackgroundType.kSolid
  }
}

function richMediaEventTypeToMojo(type: SponsoredRichMediaEventType) {
  switch (type) {
    case 'click': return NewTabPageAdEventType.kClicked
    case 'interaction': return NewTabPageAdEventType.kInteraction
    case 'mediaPlay': return NewTabPageAdEventType.kMediaPlay
    case 'media25': return NewTabPageAdEventType.kMedia25
    case 'media100': return NewTabPageAdEventType.kMedia100
  }
}

function loadWidgetPosition(): WidgetPosition {
  const value = localStorage.getItem('ntp-widget-position')
  switch (value) {
    case 'top':
    case 'bottom': return value
    default: return 'bottom'
  }
}

function storeWidgetPosition(position: WidgetPosition) {
  localStorage.setItem('ntp-widget-position', position)
}

export function createNewTabModel(): NewTabModel {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const pcdnImageURLs = new Map<string, string>()
  const sponsoredRichMediaAdEventHandler
      = SponsoredRichMediaAdEventHandler.getRemote()

  const store = createStore({
    ...defaultState(),
    backgroundsCustomizable:
        loadTimeData.getBoolean('customBackgroundFeatureEnabled')
  })

  function updateCurrentBackground() {
    store.update({
      currentBackground: getCurrentBackground(store.getState())
    })
  }

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
      braveBackgrounds: backgrounds.map((item) => ({ type: 'brave', ...item }))
    })
  }

  async function updateSelectedBackground() {
    const { background } = await handler.getSelectedBackground()
    if (background) {
      store.update({
        selectedBackgroundType: backgroundTypeFromMojo(background.type),
        selectedBackground: background.value
      })
    } else {
      store.update({
        selectedBackgroundType: 'none',
        selectedBackground: ''
      })
    }
  }

  async function updateCustomBackgrounds() {
    const { backgrounds } = await handler.getCustomBackgrounds()
    store.update({ customBackgrounds: backgrounds })
  }

  async function updateSponsoredImageBackground() {
    const { background } = await handler.getSponsoredImageBackground()
    if (!background) {
      store.update({ sponsoredImageBackground: null })
      return
    }
    let sponsoredImageBackground: SponsoredImageBackground | null = null
    if (background) {
      sponsoredImageBackground = {
        ...background,
        type: background.type === 'richMedia'
            ? 'sponsored-rich-media'
            : 'sponsored-image',
        logo: background.logo || null
      }
    }
    store.update({ sponsoredImageBackground })
  }

  async function updateClockPrefs() {
    const [
      { showClock },
      { clockFormat }
    ] = await Promise.all([
      handler.getShowClock(),
      handler.getClockFormat()
    ])

    store.update({
      showClock,
      clockFormat:
          clockFormat === 'h12' || clockFormat === 'h24' ? clockFormat : ''
    })
  }

  async function updateShieldsStats() {
    const [
      { showShieldsStats },
      { shieldsStats }
    ] = await Promise.all([
      handler.getShowShieldsStats(),
      handler.getShieldsStats()
    ])

    store.update({ showShieldsStats, shieldsStats })
  }

  async function updateTalkPrefs() {
    const { showTalkWidget } = await handler.getShowTalkWidget()
    store.update({ showTalkWidget })
  }

  function updateWidgetPosition() {
    store.update({ widgetPosition: loadWidgetPosition() })
  }

  newTabProxy.addListeners({
    onBackgroundsUpdated: debounceListener(async () => {
      await Promise.all([
        updateCustomBackgrounds(),
        updateSelectedBackground(),
      ])
      updateCurrentBackground()
    }),
    onClockStateUpdated: debounceListener(updateClockPrefs),
    onShieldsStatsUpdated: debounceListener(updateShieldsStats),
    onTalkStateUpdated: debounceListener(updateTalkPrefs)
  })

  async function loadData() {
    store.update({ widgetPosition: loadWidgetPosition() })

    await Promise.all([
      updateBackgroundsEnabled(),
      updateSponsoredImagesEnabled(),
      updateBraveBackgrounds(),
      updateCustomBackgrounds(),
      updateSelectedBackground(),
      updateSponsoredImageBackground(),
      updateWidgetPosition(),
      updateClockPrefs(),
      updateShieldsStats(),
      updateTalkPrefs()
    ])

    updateCurrentBackground()
  }

  loadData()

  return {
    getState: store.getState,

    addListener: store.addListener,

    async getPcdnImageURL(url) {
      const cachedURL = pcdnImageURLs.get(url)
      if (cachedURL) {
        return cachedURL
      }
      const { resourceData } = await handler.loadResourceFromPcdn(url)
      if (!resourceData) {
        throw new Error('Image resource could not be loaded from PCDN')
      }
      const blob = new Blob([new Uint8Array(resourceData)], { type: 'image/*' })
      const objectURL = URL.createObjectURL(blob)
      pcdnImageURLs.set(url, objectURL)
      return objectURL
    },

    setBackgroundsEnabled(enabled) {
      store.update({ backgroundsEnabled: enabled })
      handler.setBackgroundsEnabled(enabled)
    },

    setSponsoredImagesEnabled(enabled) {
      store.update({ sponsoredImagesEnabled: enabled })
      handler.setSponsoredImagesEnabled(enabled)
    },

    selectBackground(type, value) {
      store.update({
        selectedBackgroundType: type,
        selectedBackground: value
      })
      handler.selectBackground({ type: backgroundTypeToMojo(type), value })
    },

    async showCustomBackgroundChooser() {
      const { imagesSelected } = await handler.showCustomBackgroundChooser()
      return imagesSelected
    },

    async removeCustomBackground(background) {
      await handler.removeCustomBackground(background)
    },

    notifySponsoredImageLogoClicked() {
      const { sponsoredImageBackground } = store.getState()
      if (sponsoredImageBackground && sponsoredImageBackground.logo) {
        handler.notifySponsoredImageLogoClicked(
            sponsoredImageBackground.creativeInstanceId,
            sponsoredImageBackground.logo.destinationUrl,
            sponsoredImageBackground.wallpaperId);
      }
    },

    notifySponsoredRichMediaEvent(type) {
      const { sponsoredImageBackground } = store.getState()
      if (!sponsoredImageBackground) {
        return
      }
      sponsoredRichMediaAdEventHandler.reportRichMediaAdEvent(
          sponsoredImageBackground.wallpaperId,
          sponsoredImageBackground.creativeInstanceId,
          richMediaEventTypeToMojo(type));
    },

    async setShowClock(showClock) {
      await handler.setShowClock(showClock)
    },

    async setClockFormat(format) {
      await handler.setClockFormat(format)
    },

    async setShowShieldsStats(showShieldsStats) {
      await handler.setShowShieldsStats(showShieldsStats)
    },

    async setShowTalkWidget(showTalkWidget) {
      await handler.setShowTalkWidget(showTalkWidget)
    },

    async setWidgetPosition(widgetPosition) {
      storeWidgetPosition(widgetPosition)
      updateWidgetPosition()
    }
  }
}
