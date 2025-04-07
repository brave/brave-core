/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'

import { createStore } from '../lib/store'
import { optional } from '../lib/optional'
import { NewsAPI, feedItem, defaultNewsState, defaultNewsActions } from '../api/news_api'

function metadata(data: Partial<mojom.FeedItemMetadata>)
  : mojom.FeedItemMetadata
{
  return {
    title: '',
    categoryName: '',
    publisherId: '',
    publisherName: '',
    image: {
      imageUrl: { url: '' },
      paddedImageUrl: undefined
    },
    url: { url: '' },
    relativeTimeDescription: '',
    channels: [],
    publishTime: { internalValue: BigInt(0) },
    description: '',
    urlHash: '',
    score: 0,
    popScore: 0,
    ...data
  }
}

export function createNewsAPI(): NewsAPI {
  const store = createStore(defaultNewsState())

  store.update({
    newsEnabled: false,
    showNewsFeed: true,
    newsUpdateAvailable: false,
    newsFeedError: optional(),
    newsLocale: 'en-US',
    newsFeedItems: [
      feedItem({
        hero: {
          data: metadata({
            title: 'Why I chose Brave as my Chrome browser replacement',
            categoryName: 'Technology',
            publisherId: 'p1',
            publisherName: 'Tech Beat',
            url: { url: 'https://brave.com' },
            relativeTimeDescription: '2 hours ago'
          })
        }
      }),
      feedItem({
        cluster: {
          type: mojom.ClusterType.CHANNEL,
          id: 'Cluster Name',
          articles: [
            feedItem({
              article: {
                isDiscover: false,
                data: metadata({
                  title: 'Why I chose Brave as my Chrome browser replacement',
                  categoryName: 'Technology',
                  publisherId: 'p1',
                  publisherName: 'Tech Beat',
                  url: { url: 'https://brave.com' },
                  relativeTimeDescription: '2 hours ago'
                })
              }
            }),
            feedItem({
              hero: {
                data: metadata({
                  title: 'Why I chose Brave as my Chrome browser replacement',
                  categoryName: 'Technology',
                  publisherId: 'p1',
                  publisherName: 'Tech Beat',
                  url: { url: 'https://brave.com' },
                  relativeTimeDescription: '2 hours ago'
                })
              }
            })
          ]
        }
      }),
      feedItem({
        discover: {
          publisherIds: ['p1', 'p2']
        }
      })
    ],
    newsPublishers: {
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
        locales: [{
          locale: 'en-US',
          rank: 2,
          channels: []
        }]
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
        locales: [{
          locale: 'en-US',
          rank: 2,
          channels: []
        }]
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
        locales: [{
          locale: 'en-US',
          rank: 2,
          channels: []
        }]
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
        locales: [{
          locale: 'en-US',
          rank: 2,
          channels: []
        }]
      }
    },
    newsChannels: {
      'Top Stories': {
        channelName: 'Top Stories',
        subscribedLocales: []
      }
    }
  })

  return {
    getState: store.getState,

    addListener: store.addListener,

    ...defaultNewsActions(),

    setNewsEnabled(newsEnabled) {
      store.update({ newsEnabled })
    },

    setShowNewsFeed(showNewsFeed) {
      store.update({ showNewsFeed })
    },

    setNewsPublisherEnabled(publisherId, enabled) {
      const publishers = store.getState().newsPublishers
      const publisher = publishers[publisherId]
      if (publisher) {
        publisher.userEnabledStatus = enabled
          ? mojom.UserEnabled.ENABLED
          : mojom.UserEnabled.DISABLED
      }
      store.update({
        newsPublishers: { ...publishers }
      })
    },

    async findNewsFeeds(url) {
      await new Promise((resolve) => setTimeout(resolve, 2000))
      return [
        { feedTitle: 'Direct feed', feedUrl: { url: 'https://brave.com' }}
      ]
    },

    async getSuggestedNewsPublishers() {
      return ['p1', 'p2']
    },

    updateNewsFeed(options = {}) {
      if (!options.force) {
        return
      }
      const { newsFeedItems } = store.getState()
      store.update({ newsFeedItems: null })
      new Promise((resolve) => setTimeout(resolve, 2000)).then(() => {
        store.update({ newsFeedItems })
      })
    },

    onNewsVisible() {
      console.log('News visible')
    },

    notifyNewsCardVisited(cardIndex) {
      console.log('Card visited', cardIndex)
    }
  }
}
