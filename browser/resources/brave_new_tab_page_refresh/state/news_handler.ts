/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

import { Store } from '../lib/store'

import {
  NewsState,
  NewsActions,
  defaultFeedType,
  isNewsChannelEnabled,
  isNewsPublisherEnabled } from './news_state'

import {
  cacheFeed,
  loadFeedFromCache,
  cacheFeedType,
  loadFeedTypeFromCache } from './news_feed_cache'

export function createNewsHandler(
  store: Store<NewsState>
): NewsActions {
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
      feedType: loadFeedTypeFromCache() ?? defaultFeedType()
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
            isOptedIn: newsConfig.isOptedIn,
            showOnNTP: newsConfig.showOnNTP,
            feedUpdateAvailable: true,
            newsInitializing: false
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
          store.update({ feedUpdateAvailable: true })
        }
      }
    }).$.bindNewPipeAndPassRemote())
  }

  function addPublishersListener() {
    newsController.addPublishersListener(new mojom.PublishersListenerReceiver({
      changed(event) {
        const publishers = { ...store.getState().publishers }
        for (const key in event.addedOrUpdated) {
          publishers[key] = event.addedOrUpdated[key] as mojom.Publisher
        }
        for (const key of event.removed) {
          delete publishers[key]
        }
        store.update({ publishers })
        validateCurrentFeed()
      }
    }).$.bindNewPipeAndPassRemote())
  }

  function addChannelsListener() {
    newsController.addChannelsListener(new mojom.ChannelsListenerReceiver({
      changed(event) {
        const channels = { ...store.getState().channels }
        for (const key in event.addedOrUpdated) {
          channels[key] = event.addedOrUpdated[key]
        }
        for (const key of event.removed) {
          delete channels[key]
        }
        store.update({ channels })
        validateCurrentFeed()
      }
    }).$.bindNewPipeAndPassRemote())
  }

  async function updateLocale() {
    const { locale } = await newsController.getLocale()
    store.update({ newsLocale: locale })
  }

  async function updateSignals() {
    const { signals } = await newsController.getSignals()
    store.update({ signals })
  }

  async function maybeLoadFeed() {
    // Feeds can only be loaded if both prefs are enabled.
    if (!newsConfig?.isOptedIn || !newsConfig?.showOnNTP) {
      return
    }

    // Ensure that service is ready and a feed is not currently being loaded.
    if (!feedListenerAdded || feedLoading) {
      return
    }

    feedLoading = true
    store.update({ feedUpdateAvailable: false })

    const { feedType } = store.getState()

    let feed = loadFeedFromCache(feedType, currentFeedHash)
    if (!feed) {
      feed = (await fetchFeed()).feed
      cacheFeed(feed)
    }

    currentFeedHash = feed.sourceHash

    store.update({
      feedUpdateAvailable: false,
      feedItems: feed.items,
      feedError: feed.error ?? null
    })

    feedLoading = false
  }

  function fetchFeed() {
    const { feedType } = store.getState()
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
      store.update({ feedType: defaultFeedType() })
    }
  }

  function isSubscribedToCurrentFeed() {
    const { feedType } = store.getState()
    if (feedType.all || feedType.following) {
      return true
    }
    if (feedType.channel) {
      const { channels } = store.getState()
      const channel = channels[feedType.channel.channel]
      return channel && isNewsChannelEnabled(channel)
    }
    if (feedType.publisher) {
      const { publishers } = store.getState()
      const publisher = publishers[feedType.publisher.publisherId]
      return publisher && isNewsPublisherEnabled(publisher)
    }
    console.error('Unhandled feed type', feedType)
    return false
  }

  initialize()

  return {

    setIsOptedIn(isOptedIn) {
      if (!newsConfig) {
        return
      }
      newsConfig = { ...newsConfig, isOptedIn }
      newsController.setConfiguration(newsConfig)
      store.update({ isOptedIn })
    },

    setShowOnNTP(showOnNTP) {
      if (!newsConfig) {
        return
      }
      newsConfig = { ...newsConfig, showOnNTP }
      newsController.setConfiguration(newsConfig)
      store.update({ showOnNTP })
    },

    setPublisherEnabled(publisherId, enabled) {
      newsController.setPublisherPref(
        publisherId,
        enabled ? mojom.UserEnabled.ENABLED : mojom.UserEnabled.DISABLED)
    },

    setChannelSubscribed(channel, enabled) {
      const { newsLocale } = store.getState()
      newsController.setChannelSubscribed(newsLocale, channel, enabled)
    },

    subscribeToNewDirectFeed(url) {
      newsController.subscribeToNewDirectFeed({ url })
    },

    async getSuggestedPublisherIds() {
      const result = await newsController.getSuggestedPublisherIds()
      return result.suggestedPublisherIds
    },

    updateFeed(options = {}) {
      if (options.force) {
        newsController.ensureFeedV2IsUpdating()
        store.update({ feedItems: null })
        cacheFeed(null)
      }
      maybeLoadFeed()
    },

    setFeedType(feedType) {
      store.update({ feedType, feedItems: null })
      cacheFeedType(feedType)
      maybeLoadFeed()
    },

    onNewsVisible() {
      if (!newsVisible) {
        newsVisible = true
        maybeLoadFeed()
      }
    },

    async findFeeds(url) {
      const { results } = await newsController.findFeeds({ url })
      return results
    },

    onNewCardsViewed(cardIndex) {
      if (cardIndex > cardsViewedCount) {
        const newCardsViewed = cardIndex - cardsViewedCount
        cardsViewedCount = cardIndex
        newsController.onNewCardsViewed(newCardsViewed)
      }
    },

    onCardVisited(cardIndex) {
      newsController.onCardVisited(cardIndex)
    },

    onSidebarFilterUsage() {
      if (!sidebarFilterUsed) {
        sidebarFilterUsed = true
        newsController.onSidebarFilterUsage()
      }
    },

    onInteractionSessionStarted() {
      const oneHour = 1000 * 60 * 60
      if (sessionStartedAt < Date.now() - oneHour) {
        sessionStartedAt = Date.now()
        newsController.onInteractionSessionStarted()
      }
    }
  }
}
