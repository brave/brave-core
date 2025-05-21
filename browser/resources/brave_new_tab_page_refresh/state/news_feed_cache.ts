/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { mojoTimeToJSDate } from '$web-common/mojomUtils'
import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

import { newsFeedTypesEqual } from './news_state'

let cacheVersion = 1

const feedCacheKey = 'ntp-news-feed'
const feedTypeCacheKey = 'ntp-news-feed-type'
const staleFeedInterval = 1000 * 60 * 60

export function cacheFeed(feed: mojom.FeedV2 | null) {
  localStorage.setItem(feedCacheKey, stringifyData(feed))
}

export function loadFeedFromCache(
  feedType: mojom.FeedV2Type,
  expectedHash: string,
) {
  const feed = parseData<mojom.FeedV2>(localStorage.getItem(feedCacheKey))
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
  if (Date.now() - constructTime.getTime() > staleFeedInterval) {
    return true
  }
  return expectedHash && feed.sourceHash !== expectedHash
}

export function cacheFeedType(feedType: mojom.FeedV2Type) {
  localStorage.setItem(feedTypeCacheKey, stringifyData(feedType))
}

export function loadFeedTypeFromCache(): mojom.FeedV2Type | null {
  return parseData<mojom.FeedV2Type>(localStorage.getItem(feedTypeCacheKey))
}

export function setCacheVersion(n: number) {
  let prev = cacheVersion
  cacheVersion = n
  return () => {
    cacheVersion = prev
  }
}

const bigintPrefix = 'bigint:'

function stringifyData<T>(data: T) {
  return JSON.stringify({ version: cacheVersion, data }, (_, value) => {
    return typeof value === 'bigint' ? bigintPrefix + value.toString() : value
  })
}

function parseData<T>(input: string | null): T | null {
  if (!input) {
    return null
  }
  let value: any = null
  try {
    value = JSON.parse(input, (key, value) => {
      if (typeof value === 'string' && value.startsWith(bigintPrefix)) {
        value = BigInt(value.slice(bigintPrefix.length))
      }
      return value
    })
  } catch (err) {
    console.error(err)
    return null
  }
  if (!value) {
    return null
  }
  if (value.version !== cacheVersion) {
    return null
  }
  return value.data as T
}
