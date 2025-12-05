/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StateStore } from '$web-common/state_store'
import { AppState, AppActions } from './app_state'

export function createAppHandler(store: StateStore<AppState>): AppActions {
  throw new Error('Not implemented')
}
