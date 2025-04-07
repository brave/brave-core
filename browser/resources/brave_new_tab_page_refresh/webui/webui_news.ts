/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

import { createStore } from '../lib/store'
import { optional } from '../lib/optional'

import {
  NewsAPI,
  defaultFeedType,
  defaultNewsState,
  isNewsChannelEnabled,
  isNewsPublisherEnabled } from '../api/news'

import {
  cacheFeed,
  loadFeedFromCache,
  cacheFeedType,
  loadFeedTypeFromCache } from '../api/news_feed_cache'

export function createNewsAPI(): NewsAPI {
  const store = createStore(defaultNewsState())
  const newsController = mojom.BraveNewsController.getRemote()

  let newsConfig: mojom.Configuration | null = null
  let currentFeedHash = ''
  let feedLoading = false
  let newsVisible = false
  let feedListenerAdded = false
  let sidebarFilterUsed = false
  let sessionStartedAt = 0
  let cardsViewedCount = 0

  function initialize() {
    store.update({
      newsInitializing: true,
      newsFeedType: loadFeedTypeFromCache() || defaultFeedType()
    })

    addConfigurationListener()
    addPublishersListener()
    addChannelsListener()
    updateLocale()
    updateSignals()
  }

  function addConfigurationListener() {
    newsController.addConfigurationListener(
      new mojom.ConfigurationListenerReceiver({
        changed(newConfiguration) {
          newsConfig = newConfiguration
          if (newsConfig.isOptedIn) {
            addFeedListener()
          }
          store.update({
            newsEnabled: newsConfig.isOptedIn,
            showNewsWidget: newsConfig.showOnNTP,
            newsUpdateAvailable: true
          })
        }
      }).$.bindNewPipeAndPassRemote())
  }

  function addFeedListener() {
    if (feedListenerAdded) {
      return
    }
    feedListenerAdded = true
    newsController.addFeedListener(new mojom.FeedListenerReceiver({
      onUpdateAvailable(feedHash) {
        const lastFeedHash = currentFeedHash
        currentFeedHash = feedHash
        if (!lastFeedHash) {
          maybeLoadFeed()
        } else if (feedHash !== lastFeedHash) {
          store.update({ newsUpdateAvailable: true })
        }
      }
    }).$.bindNewPipeAndPassRemote())
  }

  function addPublishersListener() {
    newsController.addPublishersListener(new mojom.PublishersListenerReceiver({
      changed(event) {
        const newsPublishers = { ...store.getState().newsPublishers }
        for (const key in event.addedOrUpdated) {
          newsPublishers[key] = event.addedOrUpdated[key] as mojom.Publisher
        }
        for (const key of event.removed) {
          delete newsPublishers[key]
        }
        store.update({ newsPublishers })
        validateCurrentFeed()
      }
    }).$.bindNewPipeAndPassRemote())
  }

  function addChannelsListener() {
    newsController.addChannelsListener(new mojom.ChannelsListenerReceiver({
      changed(event) {
        const newsChannels = { ...store.getState().newsChannels }
        for (const key in event.addedOrUpdated) {
          newsChannels[key] = event.addedOrUpdated[key]
        }
        for (const key of event.removed) {
          delete newsChannels[key]
        }
        store.update({ newsChannels })
        validateCurrentFeed()

        // Once channels have loaded, we know that the News backend is
        // initialized and we can begin showing the UI.
        store.update({ newsInitializing: false })
      }
    }).$.bindNewPipeAndPassRemote())
  }

  async function updateLocale() {
    const { locale } = await newsController.getLocale()
    store.update({ newsLocale: locale })
  }

  async function updateSignals() {
    const { signals } = await newsController.getSignals()
    store.update({ newsSignals: signals })
  }

  async function maybeLoadFeed() {
    if (feedLoading || !newsConfig || !newsConfig.isOptedIn || !newsVisible) {
      return
    }

    feedLoading = true
    store.update({ newsUpdateAvailable: false })

    const { newsFeedType } = store.getState()

    let feed = loadFeedFromCache(newsFeedType, currentFeedHash)
    if (!feed) {
      feed = (await fetchFeed()).feed
      cacheFeed(feed)
    }

    currentFeedHash = feed.sourceHash

    store.update({
      newsUpdateAvailable: false,
      newsFeedItems: feed.items,
      newsFeedError: optional(feed.error ?? undefined)
    })

    feedLoading = false
  }

  function fetchFeed() {
    const feedType = store.getState().newsFeedType
    if (feedType.all) {
      return newsController.getFeedV2()
    }
    if (feedType.following) {
      return newsController.getFollowingFeed()
    }
    if (feedType.channel) {
      return newsController.getChannelFeed(feedType.channel.channel)
    }
    if (feedType.publisher) {
      return newsController.getPublisherFeed(feedType.publisher.publisherId)
    }
    console.error('Unhandled feed type', feedType)
    return newsController.getFeedV2()
  }

  function validateCurrentFeed() {
    if (!isSubscribedToCurrentFeed()) {
      store.update({ newsFeedType: defaultFeedType() })
    }
  }

  function isSubscribedToCurrentFeed() {
    const feedType = store.getState().newsFeedType
    if (feedType.all || feedType.following) {
      return true
    }
    if (feedType.channel) {
      const { newsChannels } = store.getState()
      const channel = newsChannels[feedType.channel.channel]
      return channel && isNewsChannelEnabled(channel)
    }
    if (feedType.publisher) {
      const { newsPublishers } = store.getState()
      const publisher = newsPublishers[feedType.publisher.publisherId]
      return publisher && isNewsPublisherEnabled(publisher)
    }
    console.error('Unhandled feed type', feedType)
    return false
  }

  initialize()

  return {
    getState: store.getState,

    addListener: store.addListener,

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

    setNewsChannelEnabled(channel, enabled) {
      const { newsLocale } = store.getState()
      newsController.setChannelSubscribed(newsLocale, channel, enabled)
    },

    subscribeToDirectNewsFeed(url) {
      newsController.subscribeToNewDirectFeed({ url })
    },

    async getSuggestedNewsPublishers() {
      const result = await newsController.getSuggestedPublisherIds()
      return result.suggestedPublisherIds
    },

    updateNewsFeed(options = {}) {
      if (options.force) {
        newsController.ensureFeedV2IsUpdating()
        store.update({ newsFeedItems: null })
        cacheFeed(null)
      }
      maybeLoadFeed()
    },

    setNewsFeedType(feedType) {
      store.update({
        newsFeedType: feedType,
        newsFeedItems: null
      })
      cacheFeedType(feedType)
      maybeLoadFeed()
    },

    onNewsVisible() {
      if (!newsVisible) {
        newsVisible = true
        maybeLoadFeed()
      }
    },

    async findNewsFeeds(url) {
      const { results } = await newsController.findFeeds({ url })
      return results
    },

    notifyNewsCardViewed(cardIndex) {
      const viewCount = cardIndex + 1
      if (viewCount > cardsViewedCount) {
        const newCardsViewed = viewCount - cardsViewedCount
        cardsViewedCount = viewCount
        newsController.onNewCardsViewed(newCardsViewed)
      }
    },

    notifyNewsCardVisited(cardIndex) {
      newsController.onCardVisited(cardIndex + 1)
    },

    notifyNewsSidebarFilterUsage() {
      if (!sidebarFilterUsed) {
        sidebarFilterUsed = true
        newsController.onSidebarFilterUsage()
      }
    },

    notifyNewsInteractionSessionStarted() {
      if (sessionStartedAt < Date.now() - 1000 * 60 * 60) {
        sessionStartedAt = Date.now()
        newsController.onInteractionSessionStarted()
      }
    }
  }
}
