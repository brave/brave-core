/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

import { StateStore } from '$web-common/state_store'

import {
  NewsState,
  NewsActions,
  FeedItemV2,
  ClusterType,
  feedItemMetadata,
  defaultNewsActions,
} from '../state/news_state'

function feedItem(data: Partial<FeedItemV2>) {
  return {
    article: undefined,
    advert: undefined,
    hero: undefined,
    cluster: undefined,
    discover: undefined,
    ...data,
  }
}

export function createNewsHandler(store: StateStore<NewsState>): NewsActions {
  store.update({
    initialized: true,
    newsFeatureEnabled: true,
    isOptedIn: false,
    showOnNTP: true,
    feedUpdateAvailable: false,
    feedError: null,
    newsLocale: 'en-US',
    feedItems: [
      feedItem({
        hero: {
          data: feedItemMetadata({
            title: 'Why I chose Brave as my Chrome browser replacement',
            categoryName: 'Technology',
            publisherId: 'p1',
            publisherName: 'Tech Beat',
            url: { url: 'https://brave.com' },
            relativeTimeDescription: '2 hours ago',
          }),
        },
      }),
      feedItem({
        cluster: {
          type: ClusterType.CHANNEL,
          id: 'Cluster Name',
          articles: [
            feedItem({
              article: {
                isDiscover: false,
                data: feedItemMetadata({
                  title: 'Why I chose Brave as my Chrome browser replacement',
                  categoryName: 'Technology',
                  publisherId: 'p1',
                  publisherName: 'Tech Beat',
                  url: { url: 'https://brave.com' },
                  relativeTimeDescription: '2 hours ago',
                }),
              },
            }),
            feedItem({
              hero: {
                data: feedItemMetadata({
                  title: 'Why I chose Brave as my Chrome browser replacement',
                  categoryName: 'Technology',
                  publisherId: 'p1',
                  publisherName: 'Tech Beat',
                  url: { url: 'https://brave.com' },
                  relativeTimeDescription: '2 hours ago',
                }),
              },
            }),
          ],
        },
      }),
      feedItem({
        discover: {
          publisherIds: ['p1', 'p2'],
        },
      }),
    ],
    publishers: {
      'p1': {
        publisherId: 'p1',
        publisherName: 'Publisher One',
        isEnabled: true,
        faviconUrl: { url: '' },
        type: mojom.PublisherType.COMBINED_SOURCE,
        userEnabledStatus: mojom.UserEnabled.ENABLED,
        categoryName: '',
        siteUrl: { url: '' },
        feedSource: { url: '' },
        coverUrl: { url: '' },
        backgroundColor: '',
        locales: [
          {
            locale: 'en-US',
            rank: 2,
            channels: [],
          },
        ],
      },
      'p2': {
        publisherId: 'p2',
        publisherName: 'Publisher Two',
        isEnabled: true,
        faviconUrl: { url: '' },
        type: mojom.PublisherType.DIRECT_SOURCE,
        userEnabledStatus: mojom.UserEnabled.NOT_MODIFIED,
        categoryName: '',
        siteUrl: { url: '' },
        feedSource: { url: '' },
        coverUrl: { url: '' },
        backgroundColor: '',
        locales: [
          {
            locale: 'en-US',
            rank: 2,
            channels: [],
          },
        ],
      },
      'p3': {
        publisherId: 'p3',
        publisherName: 'Publisher Three',
        isEnabled: true,
        faviconUrl: { url: '' },
        type: mojom.PublisherType.COMBINED_SOURCE,
        userEnabledStatus: mojom.UserEnabled.NOT_MODIFIED,
        categoryName: '',
        siteUrl: { url: '' },
        feedSource: { url: '' },
        coverUrl: { url: '' },
        backgroundColor: '',
        locales: [
          {
            locale: 'en-US',
            rank: 2,
            channels: [],
          },
        ],
      },
      'p4': {
        publisherId: 'p4',
        publisherName: 'Publisher Four',
        type: mojom.PublisherType.COMBINED_SOURCE,
        userEnabledStatus: mojom.UserEnabled.NOT_MODIFIED,
        isEnabled: true,
        faviconUrl: { url: '' },
        categoryName: '',
        siteUrl: { url: '' },
        feedSource: { url: '' },
        coverUrl: { url: '' },
        backgroundColor: '',
        locales: [
          {
            locale: 'en-US',
            rank: 2,
            channels: [],
          },
        ],
      },
    },
    channels: {
      'Top Stories': {
        channelName: 'Top Stories',
        subscribedLocales: [],
      },
    },
  })

  return {
    ...defaultNewsActions(),

    setIsOptedIn(isOptedIn) {
      store.update({ isOptedIn })
    },

    setShowOnNTP(showOnNTP) {
      store.update({ showOnNTP })
    },

    setPublisherEnabled(publisherId, enabled) {
      const { publishers } = store.getState()
      const publisher = publishers[publisherId]
      if (publisher) {
        publisher.userEnabledStatus = enabled
          ? mojom.UserEnabled.ENABLED
          : mojom.UserEnabled.DISABLED
      }
      store.update({
        publishers: { ...publishers },
      })
    },

    async findFeeds(url) {
      await new Promise((resolve) => setTimeout(resolve, 2000))
      return [
        { feedTitle: 'Direct feed', feedUrl: { url: 'https://brave.com' } },
      ]
    },

    async getSuggestedPublisherIds() {
      return ['p1', 'p2']
    },

    updateFeed(options = {}) {
      if (!options.force) {
        return
      }
      const { feedItems } = store.getState()
      store.update({ feedItems: null })
      new Promise((resolve) => setTimeout(resolve, 2000)).then(() => {
        store.update({ feedItems })
      })
    },

    onNewsVisible() {
      console.log('News visible')
    },

    onCardVisited(cardIndex) {
      console.log('Card visited', cardIndex)
    },
  }
}
