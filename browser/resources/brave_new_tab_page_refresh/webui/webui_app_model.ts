/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'
import { AppModel, defaultState } from '../models/app_model'
import { initializeNewTab } from './webui_new_tab'
import { initializeBackgrounds } from './webui_backgrounds'
import { initializeSearch } from './webui_search'
import { initializeTopSites } from './webui_top_sites'
import { initializeRewards } from './webui_rewards'
import { initializeVPN } from './webui_vpn'
import { initializeNews } from './webui_news'

export function createAppModel(): AppModel {
  const store = createStore(defaultState())

  return {
    getState: store.getState,
    addListener: store.addListener,

    ...initializeNewTab(store),
    ...initializeBackgrounds(store),
    ...initializeSearch(store),
    ...initializeTopSites(store),
    ...initializeRewards(store),
    ...initializeVPN(store),
    ...initializeNews(store)
  }
}
