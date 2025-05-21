/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { Publisher, FeedSearchResultItem } from '../../../state/news_state'
import { useNewsState, useNewsActions } from '../../../context/news_context'
import { getString } from '../../../lib/strings'
import { urlFromInput } from '../../../lib/url_input'
import { formatString } from '$web-common/formatString'

import {
  ChannelSourceCard,
  PublisherSourceCard,
  DirectFeedSourceCard,
} from '../source_card'

const minSourceQueryLength = 2

interface Props {
  query: string
}

export function QueryResults(props: Props) {
  const actions = useNewsActions()
  const channels = useNewsState((s) => s.channels)
  const publishers = useNewsState((s) => s.publishers)

  const [allowFindFeeds, setAllowFindFeeds] = React.useState(false)
  const [findFeedsPending, setFindFeedsPending] = React.useState(false)
  const [findFeedResults, setFindFeedResults] = React.useState<
    FeedSearchResultItem[] | null
  >(null)

  const channelMatches = React.useMemo(() => {
    return Object.values(channels).filter((channel) => {
      return queryFoundIn(props.query, [channel.channelName])
    })
  }, [props.query, channels])

  const sourceMatches = React.useMemo(() => {
    const publisherMatches: Publisher[] = []

    // If we have a viable query, add any publishers that have matches in
    // selected name or URL fields.
    if (props.query.length >= minSourceQueryLength) {
      for (const publisher of Object.values(publishers)) {
        const hasMatch = queryFoundIn(props.query, [
          publisher.publisherName,
          publisher.categoryName,
          publisher.siteUrl.url,
          publisher.feedSource.url,
        ])
        if (hasMatch) {
          publisherMatches.push(publisher)
        }
      }
    }

    const feedUrlToPublisherMap = new Map(
      Object.values(publishers).map((publisher) => [
        publisher.feedSource.url,
        publisher,
      ]),
    )

    const feedMatches: FeedSearchResultItem[] = []
    for (const result of findFeedResults ?? []) {
      // If the URL match corresponds to the feed URL of a known publisher, then
      // add the publisher to the list of publisher matches instead of the feed
      // URL matches. Otherwise, add the result to the list of feed URL matches.
      const publisher = feedUrlToPublisherMap.get(result.feedUrl.url)
      if (publisher) {
        if (!publisherMatches.includes(publisher)) {
          publisherMatches.push(publisher)
        }
      } else {
        feedMatches.push(result)
      }
    }

    return {
      publishers: publisherMatches,
      feeds: feedMatches,
    }
  }, [props.query, publishers, findFeedResults])

  React.useEffect(() => {
    let cancel = false
    const url = urlFromInput(props.query)
    if (url && allowFindFeeds) {
      setFindFeedsPending(true)
      actions.findFeeds(url.toString()).then((results) => {
        if (!cancel) {
          setFindFeedResults(results)
          setFindFeedsPending(false)
        }
      })
    } else {
      setFindFeedResults(null)
    }
    return () => {
      cancel = true
      setFindFeedsPending(false)
    }
  }, [props.query, allowFindFeeds])

  React.useEffect(() => {
    setAllowFindFeeds(false)
  }, [props.query])

  return (
    <>
      {channelMatches.length > 0 && (
        <section>
          <h4>{getString(S.BRAVE_NEWS_BROWSE_CHANNELS_HEADER)}</h4>
          <div className='source-grid'>
            {channelMatches.map((channel) => (
              <ChannelSourceCard
                key={channel.channelName}
                channel={channel}
              />
            ))}
          </div>
        </section>
      )}
      <section>
        <h4>{getString(S.BRAVE_NEWS_ALL_SOURCES_HEADER)}</h4>
        <div className='source-grid'>
          {sourceMatches.publishers.map((publisher) => (
            <PublisherSourceCard
              key={publisher.publisherId}
              publisher={publisher}
            />
          ))}
          {sourceMatches.feeds.map((feed) => (
            <DirectFeedSourceCard
              key={feed.feedUrl.url}
              title={feed.feedTitle}
              url={feed.feedUrl.url}
            />
          ))}
        </div>
        <ResultMessage
          query={props.query}
          results={findFeedResults}
          resultsPending={findFeedsPending}
          onSearchClick={() => setAllowFindFeeds(true)}
        />
      </section>
    </>
  )
}

interface ResultMessageProps {
  query: string
  results: FeedSearchResultItem[] | null
  resultsPending: boolean
  onSearchClick: () => void
}

function ResultMessage(props: ResultMessageProps) {
  const queryURL = React.useMemo(() => urlFromInput(props.query), [props.query])

  if (props.query.length < minSourceQueryLength) {
    return <p>{getString(S.BRAVE_NEWS_SEARCH_QUERY_TOO_SHORT)}</p>
  }

  if (queryURL && !props.results) {
    return (
      <div className='actions'>
        <Button
          size='small'
          onClick={() => props.onSearchClick}
          isLoading={props.resultsPending}
        >
          {formatString(getString(S.BRAVE_NEWS_DIRECT_SEARCH_BUTTON), [
            formatQueryURL(queryURL),
          ])}
        </Button>
      </div>
    )
  }

  if (queryURL && props.results && props.results.length === 0) {
    return (
      <p>
        {formatString(getString(S.BRAVE_NEWS_DIRECT_SEARCH_NO_RESULTS), [
          formatQueryURL(queryURL),
        ])}
      </p>
    )
  }
  return null
}

function queryFoundIn(query: string, subjects: string[]) {
  return subjects.some((s) => s.toLocaleLowerCase().includes(query))
}

function formatQueryURL(url: URL) {
  let urlString = url.toString()
  if (urlString.endsWith('/')) {
    urlString = urlString.slice(0, -1)
  }
  return urlString
}
