/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Input from '@brave/leo/react/input'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import PublisherCard from '../../../../../../components/brave_news/browser/resources/shared/PublisherCard'
import { getString } from '../../../lib/strings'
import { QueryResults } from './query_results'
import { SourceCarousel } from './source_carousel'
import { ChannelCard } from './channel_card'

interface Props {
  popularPublishers: mojom.Publisher[]
  suggestedPublishers: mojom.Publisher[]
  channels: Record<string, mojom.Channel>
  onShowAllPopular: () => void
  onShowAllSuggestions: () => void
}

export function DefaultPage(props: Props) {
  const [feedQuery, setFeedQuery] = React.useState('')

  return (
    <main>
      <h3>{getString('newsSettingsDiscoverTitle')}</h3>
      <Input
        placeholder={getString('newsSettingsQueryPlaceholder')}
        value={feedQuery}
        onInput={(detail) => setFeedQuery(detail.value)}
      />
      {
        feedQuery ?
          <QueryResults query={feedQuery} /> :
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
      }
    </main>
  )
}

interface PopularSectionProps {
  publishers: mojom.Publisher[]
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
        {getString('newsSettingsPopularTitle')}
        <button onClick={props.onShowAll}>
          {getString('newsViewAllButtonLabel')}
        </button>
      </h4>
      <SourceCarousel>
        {
          publishers.slice(0, 48).map((publisher) =>
            <PublisherCard
              key={publisher.publisherId}
              publisherId={publisher.publisherId}
            />
          )
        }
      </SourceCarousel>
    </section>
  )
}

interface SuggestionsSectionProps {
  publishers: mojom.Publisher[]
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
        {getString('newsSettingsSuggestionsTitle')}
        <button onClick={props.onShowAll}>
          {getString('newsViewAllButtonLabel')}
        </button>
      </h4>
      <p>{getString('newsSettingsSuggestionsText')}</p>
      <SourceCarousel>
        {
          publishers.map((publisher) =>
            <PublisherCard
              key={publisher.publisherId}
              publisherId={publisher.publisherId}
            />
          )
        }
      </SourceCarousel>
    </section>
  )
}

function ChannelsSection(props: { channels: Record<string, mojom.Channel> }) {
  const { channels } = props
  return (
    <section>
      <h4>{getString('newsSettingsChannelsTitle')}</h4>
      <div className='source-grid'>
        {
          Object.values(channels).map((channel) =>
            <ChannelCard
              key={channel.channelName}
              channelName={channel.channelName}
            />
          )
        }
      </div>
    </section>
  )
}
