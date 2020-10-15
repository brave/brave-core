// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createReducer } from 'redux-act'
import { init } from '../../actions/new_tab_actions'
import * as Actions from '../../actions/today_actions'

export type BraveTodayState = {
  // Are we in the middle of checking for new data
  isFetching: boolean | string
  // How many pages have been displayed so far for the current data
  currentPageIndex: number
  // Feed data
  feed?: BraveToday.Feed
  publishers?: BraveToday.Publishers
}

const defaultState: BraveTodayState = {
  isFetching: true,
  currentPageIndex: 0,
}

const reducer = createReducer<BraveTodayState>({}, defaultState)

export default reducer

reducer.on(init, (state, payload) => ({
  ...state,
  isFetching: true
}))

reducer.on(Actions.errorGettingDataFromBackground, (state, payload) => ({
  ...state,
  isFetching: (payload && payload.error && payload.error.message) || 'Unknown error.',
}))

reducer.on(Actions.dataReceived, (state, payload) => {
  return {
    ...state,
    isFetching: false,
    feed: payload.feed,
    publishers: payload.publishers,
    // Reset page index to ask for, even if we have current paged
    // content since feed might be new content.
    currentPageIndex: 0
  }
})

reducer.on(Actions.anotherPageNeeded, (state) => {
  // Add a new page of content to the state
  return {
    ...state,
    currentPageIndex: state.currentPageIndex + 1,
  }
})