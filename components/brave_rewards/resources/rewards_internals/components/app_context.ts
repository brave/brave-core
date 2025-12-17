/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateProvider } from '$web-common/state_provider'
import { defaultState } from '../lib/app_state'
import { createAppHandler } from '../lib/app_handler'

export const AppProvider = createStateProvider(defaultState(), createAppHandler)

export const useAppState = AppProvider.useState
export const useAppActions = AppProvider.useActions
