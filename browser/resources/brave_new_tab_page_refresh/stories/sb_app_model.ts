/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'
import { AppModel, AppState, defaultState } from '../models/app_model'
import { initializeNewTab } from './sb_new_tab'
import { initializeBackgrounds } from './sb_backgrounds'
import { initializeSearch } from './sb_search'
import { initializeTopSites } from './sb_top_sites'
import { initializeRewards } from './sb_rewards'
import { initializeVPN } from './sb_vpn'

export function createAppModel(state: Partial<AppState> = {}): AppModel {
  const store = createStore(defaultState())
  const model = {
    getState: store.getState,
    addListener: store.addListener,

    ...initializeNewTab(store),
    ...initializeBackgrounds(store),
    ...initializeSearch(store),
    ...initializeTopSites(store),
    ...initializeRewards(store),
    ...initializeVPN(store)
  }
  store.update(state)
  return model
}
