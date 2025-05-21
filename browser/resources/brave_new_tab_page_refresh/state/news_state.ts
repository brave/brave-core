/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
  FeedItemV2,
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

export interface NewsState {
  initialized: boolean
  newsFeatureEnabled: boolean
  isOptedIn: boolean
  showOnNTP: boolean
  feedType: mojom.FeedV2Type
  feedItems: mojom.FeedItemV2[] | null
  publishers: Record<string, mojom.Publisher>
  feedUpdateAvailable: boolean
  signals: Record<string, mojom.Signal>
  channels: Record<string, mojom.Channel>
  feedError: mojom.FeedV2Error | null
  newsLocale: string
}

export function defaultFeedType(): mojom.FeedV2Type {
  return feedType('all')
}

export function defaultNewsState(): NewsState {
  return {
    initialized: false,
    newsFeatureEnabled: false,
    isOptedIn: false,
    showOnNTP: false,
    feedType: defaultFeedType(),
    feedItems: null,
    publishers: {},
    feedUpdateAvailable: false,
    signals: {},
    channels: {},
    feedError: null,
    newsLocale: '',
  }
}

export interface NewsActions {
  setIsOptedIn: (isOptedIn: boolean) => void
  setShowOnNTP: (showOnNTP: boolean) => void
  setPublisherEnabled: (publisherId: string, enabled: boolean) => void
  setChannelSubscribed: (channel: string, enabled: boolean) => void
  subscribeToNewDirectFeed: (url: string) => Promise<void>
  getSuggestedPublisherIds: () => Promise<string[]>
  updateFeed: (options?: { force?: boolean }) => void
  setFeedType: (feedType: mojom.FeedV2Type) => void
  onNewsVisible: () => void
  findFeeds: (url: string) => Promise<mojom.FeedSearchResultItem[]>
  onNewCardsViewed: (cardIndex: number) => void
  onCardVisited: (cardIndex: number) => void
  onSidebarFilterUsage: () => void
  onInteractionSessionStarted: () => void
}

export function defaultNewsActions(): NewsActions {
  return {
    setIsOptedIn(isOptedIn) {},
    setShowOnNTP(showOnNTP) {},
    setPublisherEnabled(publisherId, enabled) {},
    async subscribeToNewDirectFeed(url) {},
    updateFeed(options?) {},
    setFeedType(feedType) {},
    setChannelSubscribed(channel, enabled) {},
    async getSuggestedPublisherIds() {
      return []
    },
    onNewsVisible() {},
    async findFeeds(url) {
      return []
    },
    onNewCardsViewed(cardIndex) {},
    onCardVisited(cardIndex) {},
    onSidebarFilterUsage() {},
    onInteractionSessionStarted() {},
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

export function getNewsItemImage(data: mojom.FeedItemMetadata) {
  return data.image.imageUrl?.url ?? data.image.paddedImageUrl?.url ?? ''
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

export function newsFeedTypesEqual(a: mojom.FeedV2Type, b: mojom.FeedV2Type) {
  if (a.all) {
    return Boolean(b.all)
  }
  if (a.following) {
    return Boolean(b.following)
  }
  if (a.channel) {
    return Boolean(b.channel && a.channel.channel === b.channel.channel)
  }
  if (a.publisher) {
    return Boolean(
      b.publisher && a.publisher.publisherId === b.publisher.publisherId,
    )
  }
  return false
}

export function feedType(
  key: keyof mojom.FeedV2Type,
  value: string = '',
): mojom.FeedV2Type {
  const partial = () => {
    switch (key) {
      case 'all':
        return { all: {} }
      case 'following':
        return { following: {} }
      case 'channel':
        return { channel: { channel: value } }
      case 'publisher':
        return { publisher: { publisherId: value } }
    }
  }
  return {
    all: undefined,
    following: undefined,
    publisher: undefined,
    channel: undefined,
    ...partial(),
  }
}

export function feedItemMetadata(
  data: Partial<mojom.FeedItemMetadata>,
): mojom.FeedItemMetadata {
  return {
    title: '',
    categoryName: '',
    publisherId: '',
    publisherName: '',
    image: {
      imageUrl: { url: '' },
      paddedImageUrl: undefined,
    },
    url: { url: '' },
    relativeTimeDescription: '',
    channels: [],
    publishTime: { internalValue: BigInt(0) },
    description: '',
    urlHash: '',
    score: 0,
    popScore: 0,
    ...data,
  }
}
