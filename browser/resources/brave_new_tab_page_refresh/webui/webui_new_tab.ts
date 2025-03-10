/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounceListener } from './debounce_listener'

import {
  NewTabState,
  NewTabActions,
  WidgetPosition } from '../models/new_tab'

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

export function initializeNewTab(store: Store<NewTabState>): NewTabActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const pcdnImageURLs = new Map<string, string>()

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
    onClockStateUpdated: debounceListener(updateClockPrefs),
    onShieldsStatsUpdated: debounceListener(updateShieldsStats),
    onTalkStateUpdated: debounceListener(updateTalkPrefs)
  })

  async function loadData() {
    store.update({ widgetPosition: loadWidgetPosition() })

    await Promise.all([
      updateWidgetPosition(),
      updateClockPrefs(),
      updateShieldsStats(),
      updateTalkPrefs()
    ])
  }

  loadData()

  return {

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
