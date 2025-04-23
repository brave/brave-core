/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounceListener } from './debounce_listener'
import { NewTabState, NewTabActions } from '../models/new_tab'

export function initializeNewTab(store: Store<NewTabState>): NewTabActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy

  async function updateClockPrefs() {
    const [
      { showClock },
      { clockFormat }
    ] = await Promise.all([
      handler.getShowClock(),
      handler.getClockFormat()
    ])

    store.update({ showClock, clockFormat })
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

  newTabProxy.addListeners({
    onClockStateUpdated: debounceListener(updateClockPrefs),
    onShieldsStatsUpdated: debounceListener(updateShieldsStats),
    onTalkStateUpdated: debounceListener(updateTalkPrefs)
  })

  async function loadData() {
    await Promise.all([
      updateClockPrefs(),
      updateShieldsStats(),
      updateTalkPrefs()
    ])
  }

  loadData()

  return {

    setShowClock(showClock) {
      handler.setShowClock(showClock)
    },

    setClockFormat(format) {
      handler.setClockFormat(format)
    },

    setShowShieldsStats(showShieldsStats) {
      handler.setShowShieldsStats(showShieldsStats)
    },

    setShowTalkWidget(showTalkWidget) {
      handler.setShowTalkWidget(showTalkWidget)
    }
  }
}
