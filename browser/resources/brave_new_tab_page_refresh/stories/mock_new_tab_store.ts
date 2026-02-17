/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabActions, defaultNewTabStore } from '../state/new_tab_store'

export function createNewTabStore() {
  const store = defaultNewTabStore()
  store.update({
    initialized: true,
    showClock: true,
    showShieldsStats: true,
    shieldsStats: {
      adsBlocked: 3245,
      bandwidthSavedBytes: 1024 * 1024,
    },
    showTalkWidget: true,
    talkFeatureEnabled: true,
    newsFeatureEnabled: false,
  })

  const actions: NewTabActions = {
    ...store.getState().actions,

    setClockFormat(format) {
      store.update({ clockFormat: format })
    },

    setShowClock(showClock) {
      store.update({ showClock })
    },

    setShowShieldsStats(showShieldsStats) {
      store.update({ showShieldsStats })
    },

    setShowTalkWidget(showTalkWidget) {
      store.update({ showTalkWidget })
    },
  }

  store.update({ actions })

  return store
}
