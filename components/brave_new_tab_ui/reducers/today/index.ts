// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createReducer } from 'redux-act'
import * as Actions from '../../actions/today_actions'

export type BraveTodayState = {
  // Are we in the middle of checking for new data
  isFetching: boolean | string
  isUpdateAvailable: boolean
  // How many pages have been displayed so far for the current data
  currentPageIndex: number
  cardsViewed: number
  cardsVisited: number
  // Feed data
  feed?: BraveToday.Feed
  publishers?: BraveToday.Publishers
  articleScrollTo?: BraveToday.FeedItem
}

function storeInHistoryState (data: Object) {
  const oldHistoryState = (typeof history.state === 'object') ? history.state : {}
  const newHistoryState = { ...oldHistoryState, ...data }
  history.pushState(newHistoryState, document.title)
}

const defaultState: BraveTodayState = {
  isFetching: true,
  isUpdateAvailable: false,
  currentPageIndex: 0,
  cardsViewed: 0,
  cardsVisited: 0
}
// Get previously-clicked article from history state
if (history.state && history.state.todayArticle) {
  // TODO(petemill): Type this history.state data and put in an API module
  // see `async/today`.
  defaultState.currentPageIndex = history.state.todayPageIndex as number || 0
  defaultState.articleScrollTo = history.state.todayArticle as BraveToday.FeedItem
  defaultState.cardsVisited = history.state.todayCardsVisited as number || 0
  // Clear history state now that we have the info on app state
  storeInHistoryState({
    todayArticle: null,
    todayPageIndex: null,
    todayCardsVisited: null
  })
}

// TODO(petemill): Make sure we don't keep scrolling to the scrolled-to article
// if it gets removed and rendered again (e.g. if brave today is toggled off and on).
// Reset to defaultState when Today is turned off or refreshed.

const reducer = createReducer<BraveTodayState>({}, defaultState)

export default reducer

reducer.on(Actions.interactionBegin, (state, payload) => ({
  ...state,
  isFetching: true
}))

reducer.on(Actions.errorGettingDataFromBackground, (state, payload) => ({
  ...state,
  isFetching: (payload && payload.error && payload.error.message) || 'Unknown error.'
}))

reducer.on(Actions.dataReceived, (state, payload) => {
  const newState = {
    ...state,
    isFetching: false
  }
  if (payload.feed) {
    const isNewFeed = !state.feed || state.feed.hash !== payload.feed.hash
    if (isNewFeed) {
      newState.feed = payload.feed
      newState.currentPageIndex = state.articleScrollTo ? state.currentPageIndex : 0
      newState.isUpdateAvailable = false
    }
  }
  if (payload.publishers) {
    newState.publishers = payload.publishers
  }
  return newState
})

reducer.on(Actions.anotherPageNeeded, (state) => {
  // Add a new page of content to the state
  return {
    ...state,
    currentPageIndex: state.currentPageIndex + 1
  }
})

reducer.on(Actions.readFeedItem, (state, payload) => {
  return {
    ...state,
    cardsVisited: state.cardsVisited + 1
  }
})

reducer.on(Actions.feedItemViewedCountChanged, (state, payload) => {
  // Only care if we're scrolling to new depths
  if (state.cardsViewed >= payload) {
    return state
  }
  return {
    ...state,
    cardsViewed: payload
  }
})

reducer.on(Actions.setPublisherPref, (state, payload) => {
  // Store change immediately so that we aren't relying on communication
  // with background to send us a whole updated publisher list (which it often
  // does a remote fetch in order to provide).
  let publishers = { ...state.publishers }
  let publisher = publishers[payload.publisherId]
  if (publisher) {
    publisher = {
      ...publishers[payload.publisherId],
      user_enabled: payload.enabled
    }
    publishers = {
      ...publishers,
      [payload.publisherId]: publisher
    }
  }
  return {
    ...state,
    publishers
  }
})

reducer.on(Actions.isUpdateAvailable, (state, payload) => {
  return {
    ...state,
    isUpdateAvailable: payload.isUpdateAvailable
  }
})

reducer.on(Actions.refresh, (state) => {
  return {
    ...state,
    isFetching: true,
    articleScrollTo: undefined
  }
})
