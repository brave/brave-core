/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'
import { mojoTimeToJSDate } from '$web-common/mojomUtils'

import { NewsFeedItem, NewsFeedError, NewsFeedSpecifier } from '../models/news'

interface FeedInfo {
  constructTime: number
  specifier: NewsFeedSpecifier
  items: NewsFeedItem[]
  error: NewsFeedError | null
}

export function convertNewsFeed(feed: mojom.FeedV2): FeedInfo {
  return {
    constructTime: mojoTimeToJSDate(feed.constructTime).getTime(),
    specifier: convertFeedType(feed.type),
    items: convertFeedItems(feed.items),
    error: covertFeedError(feed.error)
  }
}

function convertFeedType(type?: mojom.FeedV2Type): NewsFeedSpecifier {
  if (!type) {
    return { type: 'all' }
  }
  if (type.all) {
    return { type: 'all' }
  }
  if (type.following) {
    return { type: 'following' }
  }
  if (type.channel) {
    return { type: 'channel', channel: type.channel.channel }
  }
  if (type.publisher) {
    return { type: 'publisher', publisher: type.publisher.publisherId }
  }
  console.error('Unrecognized feed type', type)
  return { type: 'all' }
}

function convertFeedItems(items: mojom.FeedItemV2[]): NewsFeedItem[] {
  const out: NewsFeedItem[] = []
  for (const item of items) {
    const feedItem = convertFeedItem(item)
    if (feedItem) {
      out.push(feedItem)
    }
  }
  return out
}

function convertFeedItem(item: mojom.FeedItemV2): NewsFeedItem | null {
  if (item.article) {
    return {
      type: 'article',
      ...convertItemMetadata(item.article.data)
    }
  }

  if (item.hero) {
    return {
      type: 'hero',
      ...convertItemMetadata(item.hero.data)
    }
  }

  if (item.discover) {
    return {
      type: 'discover',
      publisherIds: item.discover.publisherIds
    }
  }

  if (item.cluster) {
    return {
      type: 'cluster',
      clusterType:
        item.cluster.type === mojom.ClusterType.CHANNEL ? 'channel' : 'topic',
      clusterId: item.cluster.id,
      items: convertFeedItems(item.cluster.articles as mojom.FeedItemV2[])
    }
  }

  return null
}

function convertItemMetadata(data: mojom.FeedItemMetadata) {
  return {
    title: data.title,
    categoryName: data.categoryName,
    publisherId: data.publisherId,
    publisherName: data.publisherName,
    url: data.url.url,
    imageUrl: getItemImage(data.image),
    relativeTimeDescription: data.relativeTimeDescription
  }
}

function getItemImage(image: mojom.Image) {
  if (image.paddedImageUrl) {
    return image.paddedImageUrl.url
  }
  if (image.imageUrl) {
    return image.imageUrl.url
  }
  return ''
}

function covertFeedError(value?: mojom.FeedV2Error): NewsFeedError | null {
  if (!value) {
    return null
  }
  switch (value) {
    case mojom.FeedV2Error.ConnectionError: return 'connection-error'
    case mojom.FeedV2Error.NoArticles: return 'no-articles'
    case mojom.FeedV2Error.NoFeeds: return 'no-feeds'
  }
  console.error('Unexpect feed error value', value)
  return null
}
