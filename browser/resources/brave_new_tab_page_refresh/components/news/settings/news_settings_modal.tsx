/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'

import { useLocale } from '../../context/locale_context'
import { useNewsState, useNewsActions } from '../../context/news_context'
import { FollowingList } from './following_list'
import { ChannelSourceCard, PublisherSourceCard } from '../source_card'
import { SourceCarousel } from './source_carousel'
import { QueryResults } from './query_results'
import { optional } from '../../../lib/optional'

import { style } from './news_settings_modal.style'

export type NewsSettingsView = 'default' | 'popular' | 'suggested'

interface Props {
  onClose: () => void
  initialView: NewsSettingsView
}

export function NewsSettingsModal(props: Props) {
  const { getString } = useLocale()
  const actions = useNewsActions()

  const newsLocale = useNewsState((s) => s.newsLocale)
  const publishers = useNewsState((s) => s.newsPublishers)
  const channels = useNewsState((s) => s.newsChannels)

  const [suggestedPublishers, setSuggestedPublishers] =
    React.useState<string[]>([])

  const [currentView, setCurrentView] = React.useState(props.initialView)
  const [feedQuery, setFeedQuery] = React.useState('')

  React.useEffect(() => {
    actions.getSuggestedNewsPublishers().then(setSuggestedPublishers)
  }, [])

  const popularPublishers = React.useMemo(() => {
    return Object.values(publishers)
      .map((publisher) => ({
        publisher,
        locale: publisher.locales.find((locale) => locale.locale === newsLocale)
      }))
      .filter(({ locale }) => locale)
      .sort((a, b) => {
        // A zero value signals an unranked locale. Convert to an optional to
        // keep things readable.
        const rankA = optional(a.locale!.rank || undefined)
        const rankB = optional(b.locale!.rank || undefined)

        // If neither source has a rank, sort alphabetically.
        if (!rankA.hasValue() && !rankB.hasValue()) {
          return a.publisher.publisherName.localeCompare(
            b.publisher.publisherName)
        }

        const maxRank = Number.MAX_SAFE_INTEGER
        return rankA.valueOr(maxRank) - rankB.valueOr(maxRank)
      })
      .map((item) => item.publisher)
  }, [publishers])

  function renderChannelsSection() {
    return (
      <section>
        <h4>{getString('newsSettingsChannelsTitle')}</h4>
        <div className='source-grid'>
          {
            Object.values(channels).map((channel) => (
              <ChannelSourceCard key={channel.channelName} channel={channel} />
            ))
          }
        </div>
      </section>
    )
  }

  function renderPopularPage() {
    return (
      <main>
        <SubpageHeader
          title={getString('newsSettingsPopularTitle')}
          onBack={() => setCurrentView('default')}
        />
        <div className='source-grid'>
          {
            popularPublishers.map((publisher) => (
              <PublisherSourceCard
                key={publisher.publisherId}
                publisher={publisher}
              />
            ))
          }
        </div>
      </main>
    )
  }

  function renderPopularSection() {
    if (popularPublishers.length === 0) {
      return
    }
    return (
      <section>
        <h4>
          {getString('newsSettingsPopularTitle')}
          <button onClick={() => setCurrentView('popular')}>
            {getString('newsViewAllButtonLabel')}
          </button>
        </h4>
        <SourceCarousel>
          {
            popularPublishers.slice(0, 48).map((publisher) => (
              <PublisherSourceCard
                key={publisher.publisherId}
                publisher={publisher}
              />
            ))
          }
        </SourceCarousel>
      </section>
    )
  }

  function renderSuggestedPage() {
    return (
      <main>
        <SubpageHeader
          title={getString('newsSettingsSuggestionsTitle')}
          onBack={() => setCurrentView('default')}
        />
        <div className='source-grid'>
          {
            suggestedPublishers.map((id) => (
              publishers[id] &&
                <PublisherSourceCard key={id} publisher={publishers[id]} />
            ))
          }
        </div>
      </main>
    )
  }

  function renderSuggestionsSection() {
    if (suggestedPublishers.length === 0) {
      return null
    }
    return (
      <section>
        <h4>
          {getString('newsSettingsSuggestionsTitle')}
          <button onClick={() => setCurrentView('suggested')}>
            {getString('newsViewAllButtonLabel')}
          </button>
        </h4>
        <p>{getString('newsSettingsSuggestionsText')}</p>
        <SourceCarousel>
          {
            suggestedPublishers.map((id) => (
              publishers[id] &&
                <PublisherSourceCard key={id} publisher={publishers[id]} />
            ))
          }
        </SourceCarousel>
      </section>
    )
  }

  function renderDefaultPage() {
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
              {renderPopularSection()}
              {renderSuggestionsSection()}
              {renderChannelsSection()}
            </>
        }
      </main>
    )
  }

  function renderMain() {
    switch (currentView) {
      case 'popular':
        return renderPopularPage()
      case 'suggested':
        return renderSuggestedPage()
      case 'default':
        return renderDefaultPage()
    }
  }

  return (
    <div data-css-scope={style.scope}>
      <Dialog isOpen showClose onClose={props.onClose}>
        <div className='frame'>
          <h3 className='title'>{getString('newsSettingsTitle')}</h3>
          <div className='sidebar'>
            <FollowingList />
          </div>
          {renderMain()}
        </div>
      </Dialog>
    </div>
  )
}

function SubpageHeader(props: { title: string, onBack: () => void }) {
  const { getString } = useLocale()
  return (
    <h4 className='subpage-header'>
      <span>
        <Button
          size='small'
          kind='plain-faint'
          onClick={props.onBack}
        >
          <Icon slot='icon-before' name='carat-left' className='rotate-rtl' />
          {getString('newsBackButtonLabel')}
        </Button>
      </span>
      <span>{props.title}</span>
    </h4>
  )
}
