/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'
import { AppModel, defaultState } from '../models/app_model'
import { initializeBackgrounds } from './sb_backgrounds'

export function createAppModel(): AppModel {
  const store = createStore(defaultState())

  return {
    getState: store.getState,
    addListener: store.addListener,

    ...initializeBackgrounds(store)
  }
}
