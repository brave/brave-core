/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import * as cache from './news_feed_cache'
import { feedType } from './news_state'

function mojoTime(jsTime: number) {
  return {
    internalValue: BigInt(jsTime - Date.UTC(1601, 0, 1)) * BigInt(1000),
  }
}

describe('cacheFeed', () => {
  const feed: mojom.FeedV2 = {
    constructTime: mojoTime(Date.now()),
    sourceHash: 'hash',
    type: feedType('all'),
    items: [],
    error: undefined,
  }

  it('should return null if not cached yet', () => {
    expect(cache.loadFeedFromCache(feedType('all'), 'hash')).toBe(null)
  })

  it('should return a cached feed', () => {
    cache.cacheFeed(feed)
    expect(cache.loadFeedFromCache(feedType('all'), 'hash')).toEqual(feed)
  })

  it('should return null on feed type mismatch', () => {
    expect(cache.loadFeedFromCache(feedType('following'), 'hash')).toBe(null)
  })

  it('should return null on hash mismatch', () => {
    expect(cache.loadFeedFromCache(feedType('all'), 'other-hash')).toBe(null)
  })

  it('should return null on version mismatch', () => {
    const reset = cache.setCacheVersion(0)
    expect(cache.loadFeedFromCache(feedType('all'), 'hash')).toBe(null)
    reset()
  })

  it('should return null if stale', () => {
    const staleFeed = {
      ...feed,
      constructTime: mojoTime(Date.now() - 1001 * 60 * 60),
    }
    cache.cacheFeed(staleFeed)
    expect(cache.loadFeedFromCache(feedType('all'), 'hash')).toBe(null)
  })
})

describe('cacheFeedType', () => {
  const allType = feedType('all')

  it('should correctly serialize and deserialize feed types', () => {
    expect(cache.loadFeedTypeFromCache()).toBe(null)

    cache.cacheFeedType(allType)
    expect(cache.loadFeedTypeFromCache()).toEqual(allType)

    const reset = cache.setCacheVersion(0)
    expect(cache.loadFeedTypeFromCache()).toBe(null)
    reset()
  })
})
