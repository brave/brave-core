// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { api } from '../../../../api/brave_news/news'
import { FeedSearchResultItem } from '../../../../api/brave_news'
import { useBraveNews, useChannels } from './Context'

export const QUERY_MIN_LENGTH_FILTER_SOURCES = 2

export default function useSearch (query: string) {
  const lowerQuery = query.toLowerCase()
  const channels = useChannels()
  const context = useBraveNews()
  const canQueryFilterSources = lowerQuery.length >= QUERY_MIN_LENGTH_FILTER_SOURCES
  const [directResults, setDirectResults] = React.useState<FeedSearchResultItem[]>([])
  const [loading, setLoading] = React.useState(false)
  // Whether user has asked us to perform a search at the query url
  const [canFetchUrl, setCanFetchUrl] = React.useState(false)

  // Separate results we already have a publisher for.
  type FilteredResults = {
    publisherIds: string[]
    direct: FeedSearchResultItem[]
  }

  const filteredChannels = React.useMemo(() =>
    channels.filter(c => c.channelName.toLowerCase().includes(lowerQuery)),
    [lowerQuery, channels])

  const filteredSources = React.useMemo<FilteredResults>(() => {
    const publishers = context.sortedPublishers
      .filter(p => canQueryFilterSources && (
        p.publisherName.toLowerCase().includes(lowerQuery) ||
        p.categoryName.toLocaleLowerCase().includes(lowerQuery) ||
        p.siteUrl?.url?.toLocaleLowerCase().includes(lowerQuery) ||
        p.feedSource?.url?.toLocaleLowerCase().includes(lowerQuery)))
    const results = { publishers, direct: [] as FeedSearchResultItem[] }
    for (const result of directResults) {
      const publisherMatch = publishers.find(p => p.feedSource.url === result.feedUrl.url)
      if (publisherMatch) continue

      results.direct.push(result)
    }
    return {
      publisherIds: results.publishers.map(p => p.publisherId),
      direct: results.direct
    }
  }, [directResults, context.sortedPublishers, lowerQuery])

  React.useEffect(() => {
    // Reset state when query changes
    setCanFetchUrl(false)
    setLoading(false)
    setDirectResults([])
  }, [query])

  // Contains a url as string if the query looks like a url
  const feedUrlQuery = React.useMemo<string | null>(() => {
    // Check if we have a url-able query
    let feedUrlRaw = query
    // User inputting a protocol is a signal that they want to look
    // for a feed. Otherwise we use the '.' that a hostname would have.
    // This at least makes it possible for the user to retrieve local
    // network feeds.
    let queryContainsExplicitProtocol = false
    if (feedUrlRaw.includes('://')) {
      queryContainsExplicitProtocol = true
    } else {
      // Default protocol that should catch most cases. Make sure
      // we check for validity after adding this prefix, and
      // not before.
      feedUrlRaw = 'https://' + feedUrlRaw
    }
    let isValidFeedUrl = false
    try {
      const url = new URL(feedUrlRaw)
      isValidFeedUrl = ['http:', 'https:'].includes(url.protocol) &&
        (url.hostname.includes('.') || queryContainsExplicitProtocol)
    } catch { }
    if (!isValidFeedUrl) {
      // Not a url-able query
      return null
    }
    return feedUrlRaw
  }, [query])

  React.useEffect(() => {
    // Do nothing if user hasn't asked to search, or input isn't
    // a valid Url
    if (!canFetchUrl || !feedUrlQuery) {
      console.debug('News: query changed but not searching', { canFetchUrl, feedUrlQuery })
      return
    }

    setLoading(true)
    let cancelled = false

    api.controller.findFeeds({ url: feedUrlQuery.toString() }).then(({ results }) => {
      console.debug('News: received feed results', { results, cancelled })
      if (cancelled) return

      setLoading(false)
      setDirectResults(results)
    })
    return () => {
      cancelled = true
    }
  }, [canFetchUrl, feedUrlQuery])

  return {
    canFetchUrl,
    canQueryFilterSources,
    filteredSources,
    filteredChannels,
    feedUrlQuery,
    loading,
    setCanFetchUrl
  }
}
