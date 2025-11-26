/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounce } from '$web-common/debounce'
import { NewTabState, NewTabActions } from './new_tab_state'

export function createNewTabHandler(store: Store<NewTabState>): NewTabActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy

  store.update({
    newsFeatureEnabled: loadTimeData.getBoolean('newsFeatureEnabled'),
    talkFeatureEnabled: loadTimeData.getBoolean('talkFeatureEnabled'),
  })

  async function updateClockPrefs() {
    const [{ showClock }, { clockFormat }] = await Promise.all([
      handler.getShowClock(),
      handler.getClockFormat(),
    ])

    store.update({ showClock, clockFormat })
  }

  async function updateShieldsStats() {
    const [{ showShieldsStats }, { shieldsStats }] = await Promise.all([
      handler.getShowShieldsStats(),
      handler.getShieldsStats(),
    ])

    store.update({ showShieldsStats, shieldsStats })
  }

  async function updateTalkPrefs() {
    const { showTalkWidget } = await handler.getShowTalkWidget()
    store.update({ showTalkWidget })
  }

  newTabProxy.addListeners({
    onClockStateUpdated: debounce(updateClockPrefs, 10),
    onShieldsStatsUpdated: debounce(updateShieldsStats, 10),
    onTalkStateUpdated: debounce(updateTalkPrefs, 10),
  })

  async function loadData() {
    await Promise.all([
      updateClockPrefs(),
      updateShieldsStats(),
      updateTalkPrefs(),
    ])

    store.update({ initialized: true })
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
    },
  }
}
