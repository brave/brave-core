/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Input from '@brave/leo/react/input'

import { getString } from '../../../lib/strings'
import { Publisher, Channel } from '../../../state/news_state'
import { QueryResults } from './query_results'
import { SourceCarousel } from './source_carousel'
import { PublisherSourceCard, ChannelSourceCard } from '../source_card'

interface Props {
  popularPublishers: Publisher[]
  suggestedPublishers: Publisher[]
  channels: Record<string, Channel>
  onShowAllPopular: () => void
  onShowAllSuggestions: () => void
}

export function DefaultPage(props: Props) {
  const [feedQuery, setFeedQuery] = React.useState('')

  return (
    <main>
      <h3>{getString(S.BRAVE_NEWS_DISCOVER_TITLE)}</h3>
      <Input
        placeholder={getString(S.BRAVE_NEWS_SEARCH_PLACEHOLDER_LABEL)}
        value={feedQuery}
        onInput={(detail) => setFeedQuery(detail.value)}
      />
      {feedQuery ? (
        <QueryResults query={feedQuery} />
      ) : (
        <>
          <PopularSection
            publishers={props.popularPublishers}
            onShowAll={props.onShowAllPopular}
          />
          <SuggestionsSection
            publishers={props.suggestedPublishers}
            onShowAll={props.onShowAllSuggestions}
          />
          <ChannelsSection channels={props.channels} />
        </>
      )}
    </main>
  )
}

interface PopularSectionProps {
  publishers: Publisher[]
  onShowAll: () => void
}

function PopularSection(props: PopularSectionProps) {
  const { publishers } = props
  if (publishers.length === 0) {
    return null
  }
  return (
    <section>
      <h4>
        {getString(S.BRAVE_NEWS_POPULAR_TITLE)}
        <button onClick={props.onShowAll}>
          {getString(S.BRAVE_NEWS_VIEW_ALL_BUTTON)}
        </button>
      </h4>
      <SourceCarousel>
        {publishers.slice(0, 48).map((publisher) => (
          <PublisherSourceCard
            key={publisher.publisherId}
            publisher={publisher}
          />
        ))}
      </SourceCarousel>
    </section>
  )
}

interface SuggestionsSectionProps {
  publishers: Publisher[]
  onShowAll: () => void
}

function SuggestionsSection(props: SuggestionsSectionProps) {
  const { publishers } = props
  if (publishers.length === 0) {
    return null
  }
  return (
    <section>
      <h4>
        {getString(S.BRAVE_NEWS_SUGGESTIONS_TITLE)}
        <button onClick={props.onShowAll}>
          {getString(S.BRAVE_NEWS_VIEW_ALL_BUTTON)}
        </button>
      </h4>
      <p>{getString(S.BRAVE_NEWS_SUGGESTIONS_SUBTITLE)}</p>
      <SourceCarousel>
        {publishers.map((publisher) => (
          <PublisherSourceCard
            key={publisher.publisherId}
            publisher={publisher}
          />
        ))}
      </SourceCarousel>
    </section>
  )
}

function ChannelsSection(props: { channels: Record<string, Channel> }) {
  const { channels } = props
  return (
    <section>
      <h4>{getString(S.BRAVE_NEWS_BROWSE_CHANNELS_HEADER)}</h4>
      <div className='source-grid'>
        {Object.values(channels).map((channel) => (
          <ChannelSourceCard
            key={channel.channelName}
            channel={channel}
          />
        ))}
      </div>
    </section>
  )
}
