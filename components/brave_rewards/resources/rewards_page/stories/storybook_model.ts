/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateManager } from '../../shared/lib/state_manager'
import { AppModel, AppState, defaultState, defaultModel } from '../lib/app_model'

function delay(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms)
  })
}

export function createModel(): AppModel {
  const stateManager = createStateManager<AppState>({
    ...defaultState(),
    loading: false,
    paymentId: '123'
  })

  return {
    ...defaultModel(),
    getState: stateManager.getState,
    addListener: stateManager.addListener,
    async enableRewards(countryCode) {
      await delay(500)
      setTimeout(() => {
        stateManager.update({ paymentId: 'abc123' })
      }, 20)
      return 'success'
    }
  }
}
