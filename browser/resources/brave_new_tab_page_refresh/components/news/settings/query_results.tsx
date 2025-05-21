/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import { useBraveNews } from '../../../../../../components/brave_news/browser/resources/shared/Context'
import getBraveNewsController from '../../../../../../components/brave_news/browser/resources/shared/api'
import PublisherCard, { DirectPublisherCard } from '../../../../../../components/brave_news/browser/resources/shared/PublisherCard'
import { ChannelCard } from './channel_card'
import { getString } from '../../../lib/strings'
import { urlFromInput } from '../../../lib/url_input'
import formatMessage from '$web-common/formatMessage'

const minSourceQueryLength = 2

interface Props {
  query: string
}

export function QueryResults(props: Props) {
  const braveNews = useBraveNews()
  const { channels, publishers } = braveNews

  const [allowFindFeeds, setAllowFindFeeds] = React.useState(false)
  const [findFeedsPending, setFindFeedsPending] = React.useState(false)
  const [findFeedResults, setFindFeedResults] =
    React.useState<mojom.FeedSearchResultItem[] | null>(null)

  const channelMatches = React.useMemo(() => {
    return Object.values(channels).filter((channel) => {
      return queryFoundIn(props.query, [channel.channelName])
    })
  }, [props.query, channels])

  const sourceMatches = React.useMemo(() => {
    const publisherMatches: mojom.Publisher[] = []

    // If we have a viable query, add any publishers that have matches in
    // selected name or URL fields.
    if (props.query.length >= minSourceQueryLength) {
      for (const publisher of Object.values(publishers)) {
        const hasMatch = queryFoundIn(props.query, [
          publisher.publisherName,
          publisher.categoryName,
          publisher.siteUrl.url,
          publisher.feedSource.url
        ])
        if (hasMatch) {
          publisherMatches.push(publisher)
        }
      }
    }

    const feedUrlToPublisherMap =
      new Map(Object.values(publishers).map((publisher) => [
        publisher.feedSource.url,
        publisher
      ]))

    const feedMatches: mojom.FeedSearchResultItem[] = []
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
      feeds: feedMatches
    }
  }, [props.query, publishers, findFeedResults])

  React.useEffect(() => {
    let cancel = false
    const url = urlFromInput(props.query)
    if (url && allowFindFeeds) {
      setFindFeedsPending(true)
      getBraveNewsController()
        .findFeeds({ url: url.toString() })
        .then(({ results }) => {
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

  React.useEffect(() => { setAllowFindFeeds(false) }, [props.query])

  return <>
    {
      channelMatches.length > 0 &&
        <section>
          <h4>{getString('newsSettingsChannelsTitle')}</h4>
          <div className='source-grid'>
            {
              channelMatches.map((channel) => (
                <ChannelCard
                  key={channel.channelName}
                  channelName={channel.channelName}
                />
              ))
            }
          </div>
        </section>
    }
    <section>
      <h4>{getString('newsSettingsSourcesTitle')}</h4>
      <div className='source-grid'>
        {
          sourceMatches.publishers.map((publisher) => (
            <PublisherCard
              key={publisher.publisherId}
              publisherId={publisher.publisherId}
            />
          ))
        }
        {
          sourceMatches.feeds.map((feed) => (
            <DirectPublisherCard
              key={feed.feedUrl.url}
              title={feed.feedTitle}
              feedUrl={feed.feedUrl.url}
            />
          ))
        }
      </div>
      <ResultMessage
        query={props.query}
        results={findFeedResults}
        resultsPending={findFeedsPending}
        onSearchClick={() => setAllowFindFeeds(true)}
      />
    </section>
  </>
}

interface ResultMessageProps {
  query: string
  results: mojom.FeedSearchResultItem[] | null
  resultsPending: boolean
  onSearchClick: () => void
}

function ResultMessage(props: ResultMessageProps) {
  const queryURL = React.useMemo(() => urlFromInput(props.query), [props.query])

  if (props.query.length < minSourceQueryLength) {
    return (
      <p>{getString('newsQueryTooShortText')}</p>
    )
  }

  if (queryURL && !props.results) {
    return (
      <div className='actions'>
        <Button
          size='small'
          onClick={() => props.onSearchClick}
          isLoading={props.resultsPending}
        >
          {
            formatMessage(getString('newsSearchFeedsButtonLabel'), [
              formatQueryURL(queryURL)
            ])
          }
        </Button>
      </div>
    )
  }

  if (queryURL && props.results && props.results.length === 0) {
    return (
      <p>
        {
          formatMessage(getString('newsNoMatchingFeedsText'), [
            formatQueryURL(queryURL)
          ])
        }
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
