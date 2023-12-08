// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useEffect, useState } from "react";
import getBraveNewsController, { FeedV2, FeedV2Type } from "./api";

export type FeedView = 'all' | 'following' | `publishers/${string}` | `channels/${string}`

// This is the cutoff age for loading a feed from local storage (1 hour)
const MAX_AGE_FOR_LOCAL_STORAGE_FEED = 1000 * 60 * 60

const feedTypeToFeedView = (type: FeedV2Type | undefined): FeedView => {
  if (type?.channel) return `channels/${type.channel.channel}`
  if (type?.publisher) return `publishers/${type.publisher.publisherId}`
  return 'all'
}

const FEED_KEY = 'feedV2'
const localCache: { [feedView: string]: FeedV2 } = {}
const saveFeed = (feed?: FeedV2) => {
  if (!feed) return

  localCache[feedTypeToFeedView(feed.type)] = feed

  // Note: We have to provide a replacer, because BigInt can't be serialized to JSON
  const data = JSON.stringify(feed, (_, value) => typeof value === "bigint"
    ? value.toString()
    : value);
  sessionStorage.setItem(FEED_KEY, data)
  localStorage.setItem(FEED_KEY, data)
}

const maybeLoadFeed = (view?: FeedView) => {
  const cachedFeed = localCache[view!]
  if (cachedFeed) {
    saveFeed(cachedFeed)
    return cachedFeed
  }

  // Prefer data from our current session, but fall back to whats in localStorage.
  let fromLocalStorage = false
  let data = sessionStorage.getItem(FEED_KEY)
  if (!data) {
    fromLocalStorage = true
    data = localStorage.getItem(FEED_KEY)
  }

  if (!data) return

  const feed: FeedV2 = JSON.parse(data)

  // If we loaded the feed from localStorage, and it's too old remove it from
  // storage and return undefined.
  if (fromLocalStorage
    && BigInt(feed.constructTime.internalValue) + BigInt(MAX_AGE_FOR_LOCAL_STORAGE_FEED) < Date.now()) {
    localStorage.removeItem(FEED_KEY)
    return undefined
  }

  // If the feed doesn't match what we stored, don't return it.
  return !view || feedTypeToFeedView(feed.type) === view
    ? feed
    : undefined
}

const fetchFeed = (feedView: FeedView) => {
  let promise: Promise<{ feed: FeedV2 }> | undefined
  if (feedView.startsWith('publishers/')) {
    promise = getBraveNewsController().getPublisherFeed(feedView.split('/')[1]);
  } else if (feedView.startsWith('channels/')) {
    promise = getBraveNewsController().getChannelFeed(feedView.split('/')[1])
  } else if (feedView === 'following') {
    promise = getBraveNewsController().getFollowingFeed()
  } else {
    promise = getBraveNewsController().getFeedV2()
  }

  // Make sure we add the feed to our cache
  return promise?.then(({ feed }) => {
    saveFeed(feed)
    return feed
  })
}

export const useFeedV2 = () => {
  const [feedV2, setFeedV2] = useState<FeedV2 | undefined>(maybeLoadFeed())
  const [feedView, setFeedView] = useState<FeedView>(feedTypeToFeedView(feedV2?.type))

  useEffect(() => {
    const cachedFeed = maybeLoadFeed(feedView)
    if (cachedFeed) {
      setFeedV2(cachedFeed)
      return
    }

    let cancelled = false
    fetchFeed(feedView).then((feed) => {
      if (cancelled) return
      setFeedV2(feed)
    })
    return () => { cancelled = true }
  }, [feedView])

  const refresh = useCallback(() => {
    // Set the feed to undefined - this will trigger the loading indicator.
    setFeedV2(undefined)
    getBraveNewsController().ensureFeedV2IsUpdating()
    fetchFeed(feedView).then(setFeedV2)
  }, [feedView])

  return {
    feedV2,
    feedView,
    setFeedView,
    refresh
  }
}
