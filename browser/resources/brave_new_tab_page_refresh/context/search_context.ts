/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultSearchState } from '../state/search_state'
import { createSearchHandler } from '../state/search_handler'
import { createStateProvider } from '../lib/state_provider'

export const SearchProvider =
  createStateProvider(defaultSearchState(), createSearchHandler)

export const useSearchState = SearchProvider.useState
export const useSearchActions = SearchProvider.useActions
