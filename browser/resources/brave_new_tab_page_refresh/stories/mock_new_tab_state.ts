/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateStore } from '$web-common/state_store'

import {
  NewTabActions,
  defaultNewTabState,
  defaultNewTabActions,
} from '../state/new_tab_state'

export function createNewTabState() {
  const store = createStateStore(defaultNewTabState())

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
    },
  }

  return { store, actions }
}
