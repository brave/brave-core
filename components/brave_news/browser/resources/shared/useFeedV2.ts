// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useCallback, useEffect, useState } from "react";
import getBraveNewsController, { FeedV2, FeedV2Type } from "./api";
import { addFeedListener } from "./feedListener";
import { loadTimeData } from "$web-common/loadTimeData";

export type FeedView = 'all' | 'following' | `publishers/${string}` | `channels/${string}`

// This is the cutoff age for loading a feed from local storage (1 hour)
const MAX_AGE_FOR_LOCAL_STORAGE_FEED = 1000 * 60 * 60

const feedTypeToFeedView = (type: FeedV2Type | undefined): FeedView => {
  if (type?.channel) return `channels/${type.channel.channel}`
  if (type?.publisher) return `publishers/${type.publisher.publisherId}`
  if (type?.following) return `following`
  return 'all'
}

const FEED_KEY = 'feedV2'
const FEED_TYPE_KEY = `${FEED_KEY}-type`

const localCache: { [feedView: string]: FeedV2 } = {}
const saveFeed = (feed?: FeedV2) => {
  if (!feed || !feed.items.length) {
    sessionStorage.removeItem(FEED_KEY)
    localStorage.removeItem(FEED_KEY)
    return
  }

  const type = feedTypeToFeedView(feed.type)
  localCache[type] = feed

  // Note: We have to provide a replacer, because BigInt can't be serialized to JSON
  const data = JSON.stringify(feed, (_, value) => typeof value === "bigint"
    ? value.toString()
    : value);

  try {
    sessionStorage.setItem(FEED_KEY, data)
  } catch (err) {
    console.log(err)
  }

  try {
    localStorage.setItem(FEED_KEY, data)
    localStorage.setItem(FEED_TYPE_KEY, type)
  } catch (err) {
    console.log(err)
  }
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

  // Don't load errored feeds.
  if (typeof feed.error === 'number') {
    return undefined
  }

  // If the feed doesn't match what we stored, don't return it.
  return !view || feedTypeToFeedView(feed.type) === view
    ? feed
    : undefined
}

const maybeLoadFeedView = (feed?: FeedV2): FeedView => {
  if (feed) {
    return feedTypeToFeedView(feed.type)
  }
  return localStorage.getItem(FEED_TYPE_KEY) as FeedView ?? 'all'
}

const fetchFeed = (feedView: FeedView) => {
  if (!loadTimeData.getBoolean('featureFlagBraveNewsFeedV2Enabled')) return Promise.resolve(undefined)
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

// Clear out of date caches when the feed receives new data.
addFeedListener(latestHash => {
  // Delete everything in the localCache which wasn't generated from the latest
  // data - the last visited feed is stored in under |FEED_KEY| so clicking an
  // article and coming back will still work.
  for (const key in localCache) {
    if (localCache[key].sourceHash === latestHash) continue
    delete localCache[key]
  }

  // If what's in localStorage isn't from the latest data, make sure we remove
  // it. Without the eslint-disable-next-line comment the below will fail on iOS
  // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
  const localStorageData = JSON.parse(localStorage.getItem(FEED_KEY)!) as FeedV2 | null
  if (localStorageData?.sourceHash !== latestHash) {
    localStorage.removeItem(FEED_KEY)
  }
})

export const useFeedV2 = (enabled: boolean) => {
  const [feedV2, setFeedV2] = useState<FeedV2 | undefined>(maybeLoadFeed())
  const [feedView, setFeedView] = useState<FeedView>(maybeLoadFeedView(feedV2))
  const [hash, setHash] = useState<string>()

  // Add a listener for the latest hash if Brave News is enabled. Note: We need
  // to re-add the listener when the enabled state changes because the backing
  // FeedV2Builder is created/destroyed.
  useEffect(() => {
    if (!enabled) return

    let cancelled = false
    // Note: A new feed listener will be notified with the latest hash.
    addFeedListener(newHash => {
      if (cancelled) return
      setHash(newHash)
    })

    return () => { cancelled = true }
  }, [enabled])

  useEffect(() => {
    if (!enabled) return

    setFeedV2(undefined)

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
  }, [feedView, enabled])

  const refresh = useCallback(() => {
    // Set the feed to undefined - this will trigger the loading indicator.
    setFeedV2(undefined)
    getBraveNewsController().ensureFeedV2IsUpdating()
    fetchFeed(feedView).then(setFeedV2)
  }, [feedView])

  // Updates are available if we've been told the latest hash, we have a feed
  // and the hashes don't match.
  const updatesAvailable = !!(hash && feedV2 && hash !== feedV2.sourceHash)
  return {
    feedV2,
    feedView,
    setFeedView,
    refresh,
    updatesAvailable
  }
}
