// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import usePromise from "$web-common/usePromise";
import { useEffect, useState } from "react";
import getBraveNewsController, { FeedV2, FeedV2Type } from "./api";

export type FeedView = 'all' | 'following' | `publishers/${string}` | `channels/${string}`

const feedTypeToFeedView = (type: FeedV2Type | undefined): FeedView => {
  if (type?.channel) return `channels/${type.channel.channel}`
  if (type?.publisher) return `publishers/${type.publisher.publisherId}`
  return 'all'
}

const FEED_KEY = 'feedV2'
const localCache: {[feedView: string]: FeedV2} = {}
const saveFeed = (feed?: FeedV2) => {
  if (!feed) return

  localCache[feedTypeToFeedView(feed.type)] = feed

  // Note: We have to provide a replacer, because BigInt can't be serialized to JSON
  sessionStorage.setItem(FEED_KEY, JSON.stringify(feed, (_, value) => typeof value === "bigint"
    ? value.toString()
    : value))
}

const maybeLoadFeed = (view: FeedView) => {
  const data = sessionStorage.getItem(FEED_KEY)
  if (!data) return

  const feed: FeedV2 = JSON.parse(data)

  // If the feed doesn't match what we stored, don't return it.
  return feedTypeToFeedView(feed.type) === view
    ? feed
    : undefined
}

const FEED_VIEW_KEY = 'feedV2-view'

export const useFeedV2 = () => {
  const [feedView, setFeedView] = useState<FeedView>(sessionStorage.getItem(FEED_VIEW_KEY) as any ?? 'all')
  useEffect(() => {
    sessionStorage.setItem(FEED_VIEW_KEY, feedView)
  }, [feedView])

  const { result: feedV2, loading } = usePromise<FeedV2 | undefined>(async () => {
    const localFeed = localCache[feedView]
    if (localFeed) {
      saveFeed(localFeed)
      return localFeed
    }

    const sessionFeed = maybeLoadFeed(feedView)
    if (sessionFeed) return sessionFeed

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

    return promise?.then(({ feed }) => {
      saveFeed(feed)
      return feed
    })
  }, [feedView])

  return {
    feedV2,
    feedView,
    setFeedView,
    loading
  }
}
