/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'

import {
  NewsState,
  NewsActions,
  defaultNewsActions } from '../models/news'

export function initializeNews(store: Store<NewsState>): NewsActions {
  store.update({
    showNewsWidget: true,
    newsEnabled: true,
    newsUpdateAvailable: false,
    newsFeedError: null,
    newsFeedItems: [
      {
        type: 'hero',
        title: 'Why I chose Brave as my Chrome browser replacement',
        categoryName: 'Technology',
        publisherId: 'p1',
        publisherName: 'Tech Beat',
        imageUrl: '',
        url: 'https://brave.com',
        relativeTimeDescription: '2 hours ago'
      }, {
        type: 'cluster',
        clusterType: 'channel',
        clusterId: 'Cluster Name',
        items: [
          {
            type: 'article',
            title: 'Why I chose Brave as my Chrome browser replacement',
            categoryName: 'Technology',
            publisherId: 'p1',
            publisherName: 'Tech Beat',
            imageUrl: '',
            url: 'https://brave.com',
            relativeTimeDescription: '2 hours ago'
          },
          {
            type: 'hero',
            title: 'Why I chose Brave as my Chrome browser replacement',
            categoryName: 'Technology',
            publisherId: 'p1',
            publisherName: 'Tech Beat',
            imageUrl: '',
            url: 'https://brave.com',
            relativeTimeDescription: '2 hours ago'
          }
        ]
      }, {
        type: 'discover',
        publisherIds: [
          'p1',
          'p2'
        ]
      }
    ],
    newsPublishers: {
      'p1': {
        publisherId: 'p1',
        publisherName: 'Publisher One',
        type: 'combined',
        userEnabledStatus: 'enabled',
        feedSourceUrl: '',
        coverUrl: '',
        backgroundColor: ''
      },
      'p2': {
        publisherId: 'p2',
        publisherName: 'Publisher Two',
        type: 'direct',
        userEnabledStatus: 'not-modified',
        feedSourceUrl: '',
        coverUrl: '',
        backgroundColor: ''
      }
    }
  })

  return {
    ...defaultNewsActions(),

    setShowNewsWidget(showNewsWidget) {
      store.update({ showNewsWidget })
    },

    setNewsEnabled(newsEnabled) {
      store.update({ newsEnabled })
    },

    setNewsPublisherEnabled(publisherId, enabled) {
      const publishers = store.getState().newsPublishers
      const publisher = publishers[publisherId]
      if (publisher) {
        publisher.userEnabledStatus = enabled ? 'enabled' : 'disabled'
      }
      store.update({
        newsPublishers: {...publishers}
      })
    },

    updateNewsFeed() {
      const { newsFeedItems } = store.getState()
      store.update({
        newsFeedItems: null
      })
      new Promise((resolve) => setTimeout(resolve, 2000)).then(() => {
        store.update({ newsFeedItems })
      })
    }
  }
}
