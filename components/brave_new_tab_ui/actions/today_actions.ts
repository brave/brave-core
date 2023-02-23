// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'
import * as BraveNews from '../api/brave_news'

export const interactionBegin = createAction('interactionStart')

export const ensureSettingsData = createAction('ensureSettingsData')

type DataReceivedPayload = {
  feed?: BraveNews.Feed
  publishers?: BraveNews.Publishers
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

/**
 * User has requested to read an article
 */
export type ReadFeedItemPayload = {
  item: BraveNews.FeedItem
  isPromoted?: boolean
  promotedUUID?: string
  openInNewTab?: boolean
}
export const readFeedItem = createAction<ReadFeedItemPayload>('readFeedItem')

export const feedItemViewedCountChanged = createAction<number>('feedItemViewedCountChanged')

export type PromotedItemViewedPayload = {
  item: BraveNews.FeedItem
  uuid: string
}
export const promotedItemViewed = createAction<PromotedItemViewedPayload>('promotedItemViewed')

export type VisitDisplayAdPayload = {
  ad: BraveNews.DisplayAd
  openInNewTab?: boolean
}
export const visitDisplayAd = createAction<VisitDisplayAdPayload>('visitDisplayAd')

export type DisplayAdViewedPayload = {
  ad: BraveNews.DisplayAd
}
export const displayAdViewed = createAction<DisplayAdViewedPayload>('displayAdViewed')

export type SetPublisherPrefPayload = {
  publisherId: string
  enabled: boolean | null
}
export const setPublisherPref = createAction<SetPublisherPrefPayload>('setPublisherPref', (publisherId: string, enabled: boolean | null) => ({ publisherId, enabled }))

export type RemoveDirectFeedPayload = {
  directFeed: BraveNews.Publisher
}
export const removeDirectFeed = createAction<RemoveDirectFeedPayload>('removeDirectFeed')

export const checkForUpdate = createAction('checkForUpdate')

export type IsUpdateAvailablePayload = {
  isUpdateAvailable: boolean
}
export const isUpdateAvailable = createAction<IsUpdateAvailablePayload>('isUpdateAvailable')

export const resetTodayPrefsToDefault = createAction('resetTodayPrefsToDefault')

export type RefreshPayload = {
  isFirstInteraction: boolean
} | void
export const refresh = createAction<RefreshPayload>('refresh')
