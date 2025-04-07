/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

import { Store } from '../lib/store'
import { NewsFeedSpecifier } from '../models/news'
import { NewsState, NewsActions } from '../models/news'
import { convertNewsFeed } from './news_feed_converter'

export function initializeNews(store: Store<NewsState>): NewsActions {
  const newsController = mojom.BraveNewsController.getRemote()

  let newsConfig: mojom.Configuration | null = null
  let lastFeedHash = ''
  let feedLoading = false
  let newsVisible = false

  function getFeedForSpecifier(specifier: NewsFeedSpecifier) {
    switch (specifier.type) {
      case 'all':
        return newsController.getFeedV2()
      case 'following':
        return newsController.getFollowingFeed()
      case 'channel':
        return newsController.getChannelFeed(specifier.channel)
      case 'publisher':
        return newsController.getPublisherFeed(specifier.publisher)
    }
  }

  async function loadFeed() {
    if (feedLoading || !newsConfig || !newsConfig.isOptedIn || !newsVisible) {
      return
    }

    feedLoading = true
    store.update({ newsUpdateAvailable: false })

    const { feed } = await getFeedForSpecifier(store.getState().currentNewsFeed)
    lastFeedHash = feed.sourceHash

    const feedInfo = convertNewsFeed(feed)

    store.update({
      newsUpdateAvailable: false,
      newsFeedItems: feedInfo.items,
      newsFeedError: feedInfo.error
    })

    storeFeedSpecifier(feedInfo.specifier)
    feedLoading = false
  }

  function loadFeedFromCache() {
    const specifier = loadFeedSpecifier()
    if (specifier) {
      store.update({ currentNewsFeed: specifier })
    }
  }

  newsController.addConfigurationListener(
    new mojom.ConfigurationListenerReceiver({
      changed(newConfiguration) {
        newsConfig = newConfiguration
        store.update({
          newsEnabled: newsConfig.isOptedIn,
          showNewsWidget: newsConfig.showOnNTP,
          newsOpensInNewTab: newsConfig.openArticlesInNewTab
        })
        store.update({ newsUpdateAvailable: true })
        if (!lastFeedHash) {
          loadFeed()
        }
      }
    }).$.bindNewPipeAndPassRemote())

  newsController.addFeedListener(
    new mojom.FeedListenerReceiver({
      onUpdateAvailable(feedHash) {
        if (feedHash !== lastFeedHash) {
          store.update({ newsUpdateAvailable: true })
        }
      }
    }).$.bindNewPipeAndPassRemote())

  newsController.addPublishersListener(
    new mojom.PublishersListenerReceiver({
      changed(event) {
        const newsPublishers = {
          ...store.getState().newsPublishers
        }
        for (const key in event.addedOrUpdated) {
          try {
            const entry = event.addedOrUpdated[key] as mojom.Publisher
            newsPublishers[key] = {
              publisherId: key,
              publisherName: entry.publisherName,
              type: mapPublisherType(entry.type),
              userEnabledStatus: mapUserEnabled(entry.userEnabledStatus),
              feedSourceUrl: entry.feedSource.url,
              coverUrl: entry.coverUrl?.url ?? '',
              backgroundColor: entry.backgroundColor || ''
            }
          } catch (err) {
            console.error(err)
          }
        }
        for (const key of event.removed) {
          delete newsPublishers[key]
        }
        store.update({ newsPublishers })
      }
    }).$.bindNewPipeAndPassRemote())

  newsController.addChannelsListener(
    new mojom.ChannelsListenerReceiver({
      changed(event) {
        const newsChannels = {
          ...store.getState().newsChannels
        }
        for (const key in event.addedOrUpdated) {
          try {
            const entry = event.addedOrUpdated[key] as mojom.Channel
            newsChannels[key] = {
              channelName: key,
              subscribedLocales: entry.subscribedLocales
            }
          } catch (err) {
            console.error(err)
          }
        }
        for (const key of event.removed) {
          delete newsChannels[key]
        }
        store.update({ newsChannels })
      }
    }).$.bindNewPipeAndPassRemote())

  newsController.getSignals().then((result) => {
    store.update({ newsSignals: result.signals })
  })

  loadFeedFromCache()

  return {
    setShowNewsWidget(showNewsWidget) {
      if (newsConfig) {
        newsController.setConfiguration({
          ...newsConfig,
          showOnNTP: showNewsWidget
        })
      }
    },

    setNewsEnabled(newsEnabled) {
      if (newsConfig) {
        newsController.setConfiguration({
          ...newsConfig,
          isOptedIn: newsEnabled
        })
      }
    },

    setNewsPublisherEnabled(publisherId, enabled) {
      newsController.setPublisherPref(
          publisherId,
          enabled ? mojom.UserEnabled.ENABLED : mojom.UserEnabled.DISABLED)
    },

    updateNewsFeed() {
      store.update({ newsFeedItems: null })
      loadFeed()
    },

    setCurrentNewsFeed(specifier) {
      store.update({
        currentNewsFeed: specifier,
        newsFeedItems: null
      })
      loadFeed()
    },

    onNewsVisible() {
      if (!newsVisible) {
        newsVisible = true
        loadFeed()
      }
    }
  }
}

function mapUserEnabled(value: mojom.UserEnabled) {
  switch (value) {
    case mojom.UserEnabled.DISABLED: return 'disabled'
    case mojom.UserEnabled.ENABLED: return 'enabled'
    case mojom.UserEnabled.NOT_MODIFIED: return 'not-modified'
  }
  console.error('Unexpected UserEnabled value', value)
  return 'not-modified'
}

function mapPublisherType(value: mojom.PublisherType) {
  switch (value) {
    case mojom.PublisherType.COMBINED_SOURCE: return 'combined'
    case mojom.PublisherType.DIRECT_SOURCE: return 'direct'
  }
  console.error('Unexpected PublisherType value', value)
  return 'combined'
}

const specifierStorageKey = 'ntp-news-feed-specifier'

function storeFeedSpecifier(specifier: NewsFeedSpecifier) {
  localStorage.setItem(specifierStorageKey, JSON.stringify(specifier))
}

function loadFeedSpecifier(): NewsFeedSpecifier | null {
  let value: any = null
  try { value = JSON.parse(localStorage.getItem(specifierStorageKey) || '') }
  catch {}
  switch (value?.type) {
    case 'all':
    case 'following': {
      return { type: value.type }
    }
    case 'channel': {
      const channel = String(value.channel || '')
      return channel ? { type: 'channel', channel } : null
    }
    case 'publisher': {
      const publisher = String(value.publisher || '')
      return publisher ? { type: 'publisher', publisher } : null
    }
  }
  return null
}
