/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultNewsState } from '../state/news_state'
import { createNewsHandler } from '../state/news_handler'
import { createStateProvider } from '../lib/state_provider'

export const NewsProvider = createStateProvider(
  defaultNewsState(),
  createNewsHandler,
)

export const useNewsState = NewsProvider.useState
export const useNewsActions = NewsProvider.useActions
