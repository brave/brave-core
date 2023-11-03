import usePromise from "$web-common/usePromise";
import { useEffect, useState } from "react";
import getBraveNewsController, { FeedV2, FeedV2Type } from "./api";

export type FeedView = 'all' | `publishers/${string}` | `channels/${string}`

const feedTypeToFeedView = (type: FeedV2Type): FeedView => {
  if (type.channel) return `channels/${type.channel.channel}`
  if (type.publisher) return `publishers/${type.publisher.publisherId}`
  return 'all'
}

const FEED_KEY = 'feedV2'
const FEED_VIEW_KEY = 'feedV2-view'

const saveFeed = (feed?: FeedV2) => {
  if (!feed) return

  // Note: We have to provide a replacer, because BigInt can't be serialized to JSON
  sessionStorage.setItem(FEED_KEY, JSON.stringify(feed, (_, value) => typeof value === "bigint"
    ? value.toString()
    : value))
}

const maybeLoadFeed = (view: FeedView) => {
  const data = sessionStorage.getItem(FEED_KEY)
  if (!data) return

  const feed = JSON.parse(data)

  // If the feed doesn't match what we stored, don't return it.
  return feedTypeToFeedView(feed) === view
    ? feed
    : undefined
}

export const useFeedV2 = () => {
  const [feedView, setFeedView] = useState<FeedView>(sessionStorage.getItem(FEED_VIEW_KEY) as any ?? 'all')
  useEffect(() => {
    sessionStorage.setItem(FEED_VIEW_KEY, feedView)
  }, [feedView])

  const { result: feedV2, loading } = usePromise<FeedV2 | undefined>(async () => {
    const sessionFeed = maybeLoadFeed(feedView)
    if (sessionFeed) return sessionFeed

    let promise: Promise<{ feed: FeedV2 }> | undefined
    if (feedView.startsWith('publishers/')) {
      promise = getBraveNewsController().getPublisherFeed(feedView.split('/')[1]);
    } else if (feedView.startsWith('channels/')) {
      promise = getBraveNewsController().getChannelFeed(feedView.split('/')[1])
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
