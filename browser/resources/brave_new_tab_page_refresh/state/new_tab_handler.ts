/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { StateStore } from '$web-common/state_store'
import { debounce } from '$web-common/debounce'
import { NewTabState, NewTabActions } from './new_tab_state'

export function createNewTabHandler(
  store: StateStore<NewTabState>,
): NewTabActions {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const talkFeatureEnabled = loadTimeData.getBoolean('talkFeatureEnabled')

  store.update({
    newsFeatureEnabled: loadTimeData.getBoolean('newsFeatureEnabled'),
    talkFeatureEnabled,
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
    if (talkFeatureEnabled && 'getShowTalkWidget' in handler) {
      // Cast needed because getShowTalkWidget is conditionally enabled via
      // buildflag.
      const { showTalkWidget } = await (handler as any).getShowTalkWidget()
      store.update({ showTalkWidget })
    }
  }

  const listeners: any = {
    onClockStateUpdated: debounce(updateClockPrefs, 10),
    onShieldsStatsUpdated: debounce(updateShieldsStats, 10),
  }

  if (
    talkFeatureEnabled
    && 'onTalkStateUpdated' in newTabProxy.callbackRouter
  ) {
    listeners.onTalkStateUpdated = debounce(updateTalkPrefs, 10)
  }

  newTabProxy.addListeners(listeners)

  async function loadData() {
    const promises = [updateClockPrefs(), updateShieldsStats()]

    if (talkFeatureEnabled) {
      promises.push(updateTalkPrefs())
    }

    await Promise.all(promises)

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
      if (talkFeatureEnabled && 'setShowTalkWidget' in handler) {
        // Cast needed because setShowTalkWidget is conditionally enabled via
        // buildflag.
        ;(handler as any).setShowTalkWidget(showTalkWidget)
      }
    },
  }
}
