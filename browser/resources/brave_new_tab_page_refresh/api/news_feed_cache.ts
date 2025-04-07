/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { mojoTimeToJSDate } from '$web-common/mojomUtils'
import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

import { newsFeedTypesEqual } from './news_api'

const feedCacheKey = 'ntp-news-feed'

export function cacheFeed(feed: mojom.FeedV2 | null) {
  localStorage.setItem(feedCacheKey, JSON.stringify(feed, (_, value) => {
    return typeof value === 'bigint' ? value.toString() : value
  }))
}

export function loadFeedFromCache(
  feedType: mojom.FeedV2Type,
  expectedHash: string
) {
  const storageValue = localStorage.getItem(feedCacheKey)
  if (!storageValue) {
    return null
  }
  let value: unknown = null
  try {
    value = JSON.parse(storageValue)
  } catch (err) {
    console.error(err)
  }
  const feed = value as mojom.FeedV2
  if (!feed) {
    return null
  }
  if (isStaleFeed(feed, expectedHash)) {
    return null
  }
  if (!feed.type || !newsFeedTypesEqual(feedType, feed.type)) {
    return null
  }
  return feed
}

function isStaleFeed(feed: mojom.FeedV2, expectedHash: string) {
  const constructTime = mojoTimeToJSDate(feed.constructTime)
  if (constructTime.getTime() < Date.now() - 1000 * 60 * 60) {
    return true
  }
  return expectedHash && feed.sourceHash !== expectedHash
}

const feedTypeCacheKey = 'ntp-news-feed-type'

export function cacheFeedType(feedType: mojom.FeedV2Type) {
  const json = JSON.stringify(feedType)
  localStorage.setItem(feedTypeCacheKey, json)
}

export function loadFeedTypeFromCache(): mojom.FeedV2Type | null {
  const storageValue = localStorage.getItem(feedTypeCacheKey)
  let value: unknown = null
  try { value = JSON.parse(storageValue || '') } catch {}
  if (!value) {
    return null
  }
  return value as mojom.FeedV2Type
}
