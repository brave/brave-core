/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import { useBraveNews } from '../../../../../../components/brave_news/browser/resources/shared/Context'

import { getString } from '../../../lib/strings'
import { FollowingList } from './following_list'
import { DefaultPage } from './default_page'
import { PopularPage } from './popular_page'
import { SuggestionsPage } from './suggestions_page'
import { optional } from '../../../lib/optional'

import { style } from './news_settings_modal.style'

export type NewsSettingsView = 'default' | 'popular' | 'suggested'

interface Props {
  onClose: () => void
  initialView: NewsSettingsView
}

export function NewsSettingsModal(props: Props) {
  const braveNews = useBraveNews()

  const newsLocale = braveNews.locale
  const publishers = braveNews.publishers
  const channels = braveNews.channels

  const [currentView, setCurrentView] = React.useState(props.initialView)

  const suggestedPublishers = React.useMemo(() => {
    const suggested: mojom.Publisher[] = []
    for (const id of braveNews.suggestedPublisherIds) {
      if (publishers[id]) {
        suggested.push(publishers[id])
      }
    }
    return suggested
  }, [publishers, braveNews.suggestedPublisherIds])

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

  function pageView() {
    switch (currentView) {
      case 'popular':
        return (
          <PopularPage
            publishers={popularPublishers}
            onBack={() => setCurrentView('default')}
          />
        )
      case 'suggested':
        return (
          <SuggestionsPage
            publishers={suggestedPublishers}
            onBack={() => setCurrentView('default')}
          />
        )
      case 'default':
        return (
          <DefaultPage
            popularPublishers={popularPublishers}
            suggestedPublishers={suggestedPublishers}
            channels={channels}
            onShowAllPopular={() => setCurrentView('popular')}
            onShowAllSuggestions={() => setCurrentView('suggested')}
          />
        )
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
          {pageView()}
        </div>
      </Dialog>
    </div>
  )
}
