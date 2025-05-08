/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useLocale } from '../../context/locale_context'
import { Publisher, FeedSearchResultItem } from '../../../api/news'
import { useNewsState, useNewsActions } from '../../context/news_context'
import { urlFromInput } from '../../../lib/url_input'
import formatMessage from '$web-common/formatMessage'

import {
  ChannelSourceCard,
  PublisherSourceCard,
  DirectFeedSourceCard } from '../source_card'

const minSourceQueryLength = 2

interface Props {
  query: string
}

export function QueryResults(props: Props) {
  const { getString } = useLocale()
  const actions = useNewsActions()
  const channels = useNewsState((s) => s.newsChannels)
  const publishers = useNewsState((s) => s.newsPublishers)

  const [allowFindFeeds, setAllowFindFeeds] = React.useState(false)
  const [findFeedsPending, setFindFeedsPending] = React.useState(false)
  const [findFeedResults, setFindFeedResults] =
    React.useState<FeedSearchResultItem[] | null>(null)

  const queryURL = React.useMemo(() => urlFromInput(props.query), [props.query])

  const channelMatches = React.useMemo(() => {
    return Object.values(channels).filter((channel) => {
      return queryFoundIn(props.query, [channel.channelName])
    })
  }, [props.query, channels])

  const sourceMatches = React.useMemo(() => {
    const publisherMatches: Publisher[] = []
    if (props.query.length >= minSourceQueryLength) {
      publisherMatches.push(...Object.values(publishers).filter((publisher) => {
        return queryFoundIn(props.query, [
          publisher.publisherName,
          publisher.categoryName,
          publisher.siteUrl.url,
          publisher.feedSource.url
        ])
      }))
    }

    const publisherFeedUrls =
      new Map(Object.values(publishers).map((publisher) => [
        publisher.feedSource.url,
        publisher
      ]))

    const feeds: FeedSearchResultItem[] = []
    for (const result of findFeedResults || []) {
      // If the query corresponds to a known publisher, then add the publisher
      // to the list of publisher results instead of the URL match results.
      const publisher = publisherFeedUrls.get(result.feedUrl.url)
      if (publisher) {
        if (!publisherMatches.includes(publisher)) {
          publisherMatches.push(publisher)
        }
      } else {
        feeds.push(result)
      }
    }

    return {
      publishers: publisherMatches,
      feeds
    }
  }, [props.query, publishers, findFeedResults])

  React.useEffect(() => {
    let cancel = false
    const url = urlFromInput(props.query)
    if (url && allowFindFeeds) {
      setFindFeedsPending(true)
      actions.findNewsFeeds(url.toString()).then((results) => {
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

  function renderSourceResultMessages() {
    if (props.query.length < minSourceQueryLength) {
      return (
        <p>{getString('newsQueryTooShortText')}</p>
      )
    }
    if (queryURL && !findFeedResults) {
      return (
        <div className='actions'>
          <Button
            size='small'
            onClick={() => setAllowFindFeeds(true)}
            isLoading={findFeedsPending}
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
    if (queryURL && findFeedResults && findFeedResults.length === 0) {
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

  return <>
    {
      channelMatches.length > 0 &&
        <section>
          <h4>{getString('newsSettingsChannelsTitle')}</h4>
          <div className='source-grid'>
            {
              channelMatches.map((channel) => (
                <ChannelSourceCard
                  key={channel.channelName}
                  channel={channel}
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
            <PublisherSourceCard
              key={publisher.publisherId}
              publisher={publisher}
            />
          ))
        }
        {
          sourceMatches.feeds.map((feed) => (
            <DirectFeedSourceCard
              key={feed.feedUrl.url}
              title={feed.feedTitle}
              url={feed.feedUrl.url}
            />
          ))
        }
      </div>
      {renderSourceResultMessages()}
    </section>
  </>
}

function queryFoundIn(query: string, subjects: string[]) {
  return subjects.some((s) => s.toLocaleLowerCase().includes(query))
}

function formatQueryURL(url: URL) {
  return url.toString().replace(/\/+$/, '')
}
