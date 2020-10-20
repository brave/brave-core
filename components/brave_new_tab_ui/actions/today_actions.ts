// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'

export const init = createAction<void>('init')

type DataReceivedPayload = {
  feed: BraveToday.Feed | undefined
  publishers: BraveToday.Publishers | undefined
}
export const dataReceived = createAction<DataReceivedPayload>('dataReceived')

/**
 * Scroll has reached a position so that another page of content is needed
 */
export const anotherPageNeeded = createAction('anotherPageNeeded')

type BackgroundErrorPayload = {
  error: Error
}
export const errorGettingDataFromBackground = createAction<BackgroundErrorPayload>('errorGettingDataFromBackground')

/** User has requested to read an article */
export type ReadFeedItemPayload = BraveToday.FeedItem
export const readFeedItem = createAction<ReadFeedItemPayload>('readFeedItem')
