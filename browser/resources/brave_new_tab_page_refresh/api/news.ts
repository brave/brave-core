/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { API } from '../lib/api'
import { Optional, optional } from '../lib/optional'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

export {
  Publisher,
  Channel,
  Signal,
  FeedItemMetadata,
  FeedV2Type,
  FeedV2Error,
  Cluster,
  ClusterType,
  FeedSearchResultItem,
  FeedItemV2 } from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

export interface NewsState {
  newsInitializing: boolean
  showNewsWidget: boolean
  newsEnabled: boolean
  newsFeedType: mojom.FeedV2Type
  newsFeedItems: mojom.FeedItemV2[] | null
  newsPublishers: Record<string, mojom.Publisher>
  newsUpdateAvailable: boolean
  newsSignals: Record<string, mojom.Signal>
  newsChannels: Record<string, mojom.Channel>
  newsFeedError: Optional<mojom.FeedV2Error>
  newsLocale: string
}

export function defaultFeedType(): mojom.FeedV2Type {
  return feedType('all')
}

export function defaultNewsState(): NewsState {
  return {
    newsInitializing: false,
    showNewsWidget: false,
    newsEnabled: false,
    newsFeedType: defaultFeedType(),
    newsFeedItems: null,
    newsPublishers: {},
    newsUpdateAvailable: false,
    newsSignals: {},
    newsChannels: {},
    newsFeedError: optional(),
    newsLocale: ''
  }
}

export interface NewsActions {
  setShowNewsWidget: (showNewsWidget: boolean) => void
  setNewsEnabled: (newsEnabled: boolean) => void
  setNewsPublisherEnabled: (publisherId: string, enabled: boolean) => void
  setNewsChannelEnabled: (channel: string, enabled: boolean) => void
  subscribeToDirectNewsFeed: (url: string) => void
  getSuggestedNewsPublishers: () => Promise<string[]>
  updateNewsFeed: (options?: { force?: boolean }) => void
  setNewsFeedType: (feedType: mojom.FeedV2Type) => void
  onNewsVisible: () => void
  findNewsFeeds: (url: string) => Promise<mojom.FeedSearchResultItem[]>
  notifyNewsCardViewed: (cardIndex: number) => void
  notifyNewsCardVisited: (cardIndex: number) => void
  notifyNewsSidebarFilterUsage: () => void
  notifyNewsInteractionSessionStarted: () => void
}

export function defaultNewsActions(): NewsActions {
  return {
    setShowNewsWidget(showNewsWidget) {},
    setNewsEnabled(newsEnabled) {},
    setNewsPublisherEnabled(publisherId, enabled) {},
    subscribeToDirectNewsFeed(url) {},
    updateNewsFeed(options?) {},
    setNewsFeedType(specifier) {},
    setNewsChannelEnabled(channel, enabled) {},
    async getSuggestedNewsPublishers() { return [] },
    onNewsVisible() {},
    async findNewsFeeds(url) { return [] },
    notifyNewsCardViewed(cardIndex) {},
    notifyNewsCardVisited(cardIndex) {},
    notifyNewsSidebarFilterUsage() {},
    notifyNewsInteractionSessionStarted() {}
  }
}

export function getNewsPublisherName(item: mojom.FeedItemMetadata) {
  if (item.publisherName) {
    return item.publisherName
  }
  try {
    return new URL(item.url.url).hostname
  } catch {
    return ''
  }
}

export function isNewsChannelEnabled(channel: mojom.Channel) {
  return channel.subscribedLocales.length > 0
}

export function isNewsPublisherEnabled(publisher: mojom.Publisher) {
  switch (publisher.userEnabledStatus) {
    case mojom.UserEnabled.ENABLED:
      return true
    case mojom.UserEnabled.DISABLED:
      return false
    case mojom.UserEnabled.NOT_MODIFIED:
      return publisher.type === mojom.PublisherType.DIRECT_SOURCE
    default:
      console.error('Unhandled UserEnabled value', publisher.userEnabledStatus)
      return false
  }
}

type FeedTypeKey = keyof mojom.FeedV2Type

export function feedType(key: FeedTypeKey, value: string =  '')
  : mojom.FeedV2Type
{
  return {
    all: undefined,
    following: undefined,
    publisher: undefined,
    channel: undefined,
    ...(
      key === 'all' ? { all: {} } :
      key === 'following' ? { following: {} } :
      key === 'channel' ? { channel: { channel: value } } :
      key === 'publisher' ? { publisher: { publisherId: value } } :
      null
    )
  }
}

export function newsItemImage(data: mojom.FeedItemMetadata) {
  return data.image.imageUrl?.url ?? data.image.paddedImageUrl?.url ?? ''
}

export function newsFeedTypesEqual(a: mojom.FeedV2Type, b: mojom.FeedV2Type) {
  if (a.all) {
    return b.all
  }
  if (a.following) {
    return b.following
  }
  if (a.channel) {
    return b.channel && a.channel.channel === b.channel.channel
  }
  if (a.publisher) {
    return b.publisher && a.publisher.publisherId === b.publisher.publisherId
  }
  return false
}

export function feedItem(item: Partial<mojom.FeedItemV2>): mojom.FeedItemV2 {
  return {
    article: undefined,
    advert: undefined,
    hero: undefined,
    cluster: undefined,
    discover: undefined,
    ...item
  }
}

export type NewsAPI = API<NewsState, NewsActions>
