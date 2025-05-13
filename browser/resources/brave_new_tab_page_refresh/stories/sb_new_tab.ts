/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'

import {
  NewTabAPI,
  defaultNewTabState,
  defaultNewTabActions } from '../context/new_tab'

export function createNewTabAPI(): NewTabAPI {
  const store = createStore(defaultNewTabState())

  store.update({
    showClock: true,
    showShieldsStats: true,
    shieldsStats: {
      adsBlocked: 3245,
      bandwidthSavedBytes: 1024 * 1024
    },
    showTalkWidget: true
  })

  return {
    getState: store.getState,

    addListener: store.addListener,

    ...defaultNewTabActions(),

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
    }
  }
}
