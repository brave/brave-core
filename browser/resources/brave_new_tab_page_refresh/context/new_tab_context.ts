/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultNewTabState } from '../state/new_tab_state'
import { createNewTabHandler } from '../state/new_tab_handler'
import { createStateProvider } from '../lib/state_provider'

export const NewTabProvider = createStateProvider(
  defaultNewTabState(),
  createNewTabHandler,
)

export const useNewTabState = NewTabProvider.useState
export const useNewTabActions = NewTabProvider.useActions
