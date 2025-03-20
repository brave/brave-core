/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  NewTabState,
  NewTabActions,
  defaultNewTabActions } from '../models/new_tab'

export function initializeNewTab(store: Store<NewTabState>): NewTabActions {
  store.update({
    showClock: true
  })

  return {
    ...defaultNewTabActions(),

    setClockFormat(format) {
      store.update({ clockFormat: format })
    },

    setShowClock(showClock) {
      store.update({ showClock })
    }
  }
}
