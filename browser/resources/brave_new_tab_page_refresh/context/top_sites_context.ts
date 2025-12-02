/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultTopSitesState } from '../state/top_sites_state'
import { createTopSitesActions } from '../state/top_sites_actions'
import { createStateProvider } from '../lib/state_provider'

export const TopSitesProvider = createStateProvider(
  defaultTopSitesState(),
  createTopSitesActions,
)

export const useTopSitesState = TopSitesProvider.useState
export const useTopSitesActions = TopSitesProvider.useActions
