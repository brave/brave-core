// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { withKnobs, text } from '@storybook/addon-knobs'
import ThemeProvider from '../../common/BraveCoreThemeProvider'
import BraveNewsLoadingCard from '../components/default/braveNews/cards/cardLoading'
import BraveNewsErrorCard from '../components/default/braveNews/cards/cardError'
import BraveNewsOptInCard from '../components/default/braveNews/cards/cardOptIn'
import PublisherMeta from '../components/default/braveNews/cards/PublisherMeta'
import DisplayAdCard from '../components/default/braveNews/cards/displayAd'
import * as BraveNews from '../../brave_news/browser/resources/shared/api'
import getBraveNewsDisplayAd from './default/data/getBraveNewsDisplayAd'
import './todayStrings'

const onClick = () => alert('clicked')

export default {
  title: 'New Tab/Brave News',
  decorators: [
    (Story: any) => <ThemeProvider><Story /></ThemeProvider>,
    (Story: any) => (
      <div
        style={{
          display: 'flex',
          fontFamily: 'Poppins',
          alignItems: 'center',
          justifyContent: 'center',
          gap: '20px',
          width: '100%',
          minHeight: '100vh',
          background: 'url(https://placekitten.com/2000/3000)',
          backgroundSize: 'contain',
          color: 'pink',
          fontSize: '22px',
          fontWeight: 600
        }}
      >
        <Story />
      </div>
    ),
    withKnobs
  ]
}

export const Publisher = () => (
  <>
    <PublisherMeta
      publisher={{
        publisherId: '123abc',
        publisherName: text('Publisher Name', 'small'),
        categoryName: 'Top News',
        feedSource: { url: 'http://www.example.com/feed' },
        backgroundColor: undefined,
        coverUrl: undefined,
        faviconUrl: undefined,
        siteUrl: { url: 'https://www.example.com' },
        locales: [{
          locale: 'en_US',
          rank: 0,
          channels: ['Top News', 'Top Sources']
        }],
        type: BraveNews.PublisherType.COMBINED_SOURCE,
        isEnabled: true,
        userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
      }}
      onSetPublisherPref={onClick}
    />
    <PublisherMeta
      publisher={{
        publisherId: '123abcdef',
        publisherName: text('Publisher Name 2', 'The Miller Chronicle'),
        categoryName: 'Top News',
        feedSource: { url: 'http://www.example.com/feed' },
        backgroundColor: undefined,
        coverUrl: undefined,
        faviconUrl: undefined,
        siteUrl: { url: 'https://www.example.com' },
        locales: [{
          locale: 'en_US',
          rank: 0,
          channels: ['Top News', 'Top Sources']
        }],
        type: BraveNews.PublisherType.COMBINED_SOURCE,
        isEnabled: true,
        userEnabledStatus: BraveNews.UserEnabled.NOT_MODIFIED
      }}
      onSetPublisherPref={onClick}
    />
  </>
)

export const Loading = () => (
  <BraveNewsLoadingCard />
)

export const Error = () => (
  <BraveNewsErrorCard onRefresh={() => console.log('refresh clicked')} />
)

export const OptIn = () => (
  <BraveNewsOptInCard onOptIn={() => console.log('opt-in clicked')} onDisable={() => console.log('disable clicked')} />
)

const handleDisplayAdVisit = () => alert('handle visit')
const handleDisplayAdView = () => console.log('display ad viewed')

export const DisplayAd = () => (
  <DisplayAdCard
    getContent={getBraveNewsDisplayAd.bind(undefined, true)}
    onVisitDisplayAd={handleDisplayAdVisit}
    onViewedDisplayAd={handleDisplayAdView}
  />
)
