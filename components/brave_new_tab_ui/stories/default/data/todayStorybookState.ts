// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { boolean } from '@storybook/addon-knobs'
import { BraveNewsState } from '../../../reducers/today'
import { feed, publishers } from './mockBraveNewsController'

export default function getTodayState (): BraveNewsState {
  const hasDataError = boolean('Today data fetch error?', false)
  return {
    isFetching: boolean('Today is fetching?', false),
    hasInteracted: boolean('Today has interacted?', false),
    isUpdateAvailable: boolean('Is Today update available?', false),
    currentPageIndex: 10,
    cardsViewed: 0,
    cardsViewedDelta: 0,
    cardsVisited: 0,
    publishers: hasDataError ? undefined : publishers,
    feed: hasDataError ? undefined : feed
  }
}
