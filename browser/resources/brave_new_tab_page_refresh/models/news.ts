/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export interface NewsItem {
  title: string
  categoryName: string
  publisherId: string
  publisherName: string
  url: string
  imageUrl: string
  relativeTimeDescription: string
}

type ArticleFeedItem = NewsItem & { type: 'article' }

type HeroFeedItem = NewsItem & { type: 'hero' }

interface DiscoverFeedItem {
  type: 'discover'
  publisherIds: string[]
}

export interface ClusterFeedItem {
  type: 'cluster'
  clusterType: 'channel' | 'topic'
  clusterId: string
  items: NewsFeedItem[]
}

export type NewsFeedItem =
  ArticleFeedItem |
  HeroFeedItem |
  DiscoverFeedItem |
  ClusterFeedItem

interface AllFeedSpecifier {
  type: 'all'
}

interface FollowingFeedSpecifier {
  type: 'following'
}

interface PublisherFeedSpecifier {
  type: 'publisher'
  publisher: string
}

interface ChannelFeedSpecifier {
  type: 'channel'
  channel: string
}

export interface NewsPublisher {
  publisherId: string
  publisherName: string
  type: 'combined' | 'direct'
  userEnabledStatus: 'not-modified' | 'enabled' | 'disabled'
  feedSourceUrl: string
  coverUrl: string
  backgroundColor: string
}

export type NewsFeedSpecifier =
  AllFeedSpecifier |
  FollowingFeedSpecifier |
  PublisherFeedSpecifier |
  ChannelFeedSpecifier

export interface NewsSignal {
  visitWeight: number
}

export type NewsFeedError = 'no-articles' | 'no-feeds' | 'connection-error'

interface NewsChannel {
  channelName: string
  subscribedLocales: string[]
}

export interface NewsState {
  showNewsWidget: boolean
  newsEnabled: boolean
  newsOpensInNewTab: boolean
  currentNewsFeed: NewsFeedSpecifier
  newsFeedItems: NewsFeedItem[] | null
  newsPublishers: Record<string, NewsPublisher>
  newsUpdateAvailable: boolean
  newsSignals: Record<string, NewsSignal>
  newsChannels: Record<string, NewsChannel>
  newsFeedError: NewsFeedError | null
}

export function defaultNewsState(): NewsState {
  return {
    showNewsWidget: false,
    newsEnabled: false,
    newsOpensInNewTab: false,
    currentNewsFeed: { type: 'all' },
    newsFeedItems: null,
    newsPublishers: {},
    newsUpdateAvailable: false,
    newsSignals: {},
    newsChannels: {},
    newsFeedError: null
  }
}

export interface NewsActions {
  setShowNewsWidget: (showNewsWidget: boolean) => void
  setNewsEnabled: (newsEnabled: boolean) => void
  setNewsPublisherEnabled: (publisherId: string, enabled: boolean) => void
  updateNewsFeed: () => void
  setCurrentNewsFeed: (specifier: NewsFeedSpecifier) => void
  onNewsVisible: () => void
}

export function defaultNewsActions(): NewsActions {
  return {
    setShowNewsWidget(showNewsWidget) {},
    setNewsEnabled(newsEnabled) {},
    setNewsPublisherEnabled(publisherId, enabled) {},
    updateNewsFeed() {},
    setCurrentNewsFeed(specifier) {},
    onNewsVisible() {}
  }
}

export function getNewsPublisherName(item: NewsItem) {
  if (item.publisherName) {
    return item.publisherName
  }
  try {
    return new URL(item.url).hostname
  } catch {
    return ''
  }
}

export function isNewsPublisherEnabled(publisher: NewsPublisher) {
  switch (publisher.userEnabledStatus) {
    case 'enabled': return true
    case 'disabled': return false
    case 'not-modified': return publisher.type === 'direct'
  }
}

export function newsFeedSpecifiersEqual(
  a: NewsFeedSpecifier,
  b: NewsFeedSpecifier
) {
  switch (a.type) {
    case 'all':
    case 'following':
      return a.type === b.type
    case 'channel':
      return b.type === 'channel' && a.channel === b.channel
    case 'publisher':
      return b.type === 'publisher' && a.publisher === b.publisher
  }
}
