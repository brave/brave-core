/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import type {
  Configuration,
  FeedV2,
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

import { ConfigurationCachingWrapper } from '../../../../components/brave_news/browser/resources/shared/configurationCache'

const sampleImageUrl =
  'https://brave.com/static-assets/images/coding-background-texture.jpg'

const sampleFeedV2: FeedV2 = {
  constructTime: { internalValue: BigInt(Date.now()) * BigInt(1000) },
  sourceHash: 'storybook-feed-v2',
  type: { all: {} },
  items: [
    {
      article: {
        isDiscover: false,
        data: {
          categoryName: 'Tech',
          channels: ['Tech'],
          publishTime: { internalValue: BigInt(Date.now()) * BigInt(1000) },
          title: 'Brave launches refreshed new tab page',
          description: 'A sample article for Storybook.',
          url: { url: 'https://brave.com' },
          urlHash: '',
          image: {
            imageUrl: { url: sampleImageUrl },
            paddedImageUrl: undefined,
          },
          publisherId:
            '5eece347713f329f156cd0204cf9b12629f1dc8f4ea3c1b67984cfbfd66cdca5',
          publisherName: 'Brave',
          score: 1,
          popScore: 0,
          relativeTimeDescription: '1 hour ago',
        },
      },
    },
  ],
  error: undefined,
}

let configuration: Configuration = {
  isOptedIn: false,
  showOnNTP: false,
  openArticlesInNewTab: true,
}

let newsFeedV2FeatureFlagEnabled = false

export function setupBraveNewsStorybookMock() {
  // @ts-expect-error
  const controller = window.storybookBraveNewsController
  if (!controller || controller.__ntpStorybookConfigured) {
    return
  }

  Object.assign(controller, {
    __ntpStorybookConfigured: true,

    async getConfiguration() {
      return { configuration: { ...configuration } }
    },

    async setConfiguration(config: Configuration) {
      configuration = { ...configuration, ...config }
      ConfigurationCachingWrapper.getInstance().changed(configuration)
      return {}
    },

    async getFeedV2() {
      return { feed: sampleFeedV2 }
    },

    async getFollowingFeed() {
      return { feed: sampleFeedV2 }
    },

    async getPublisherFeed() {
      return { feed: sampleFeedV2 }
    },

    async getChannelFeed() {
      return { feed: sampleFeedV2 }
    },

    async ensureFeedV2IsUpdating() {},
  })
}

export function setBraveNewsStoryConfiguration(change: Partial<Configuration>) {
  configuration = { ...configuration, ...change }
  ConfigurationCachingWrapper.getInstance().changed(configuration)
}

export function setBraveNewsFeedV2FeatureFlagEnabled(enabled: boolean) {
  newsFeedV2FeatureFlagEnabled = enabled

  const global = window as typeof window & {
    loadTimeData: {
      getBoolean: (key: string) => boolean
      __ntpStorybookOriginalGetBoolean?: (key: string) => boolean
    }
  }

  if (!global.loadTimeData.__ntpStorybookOriginalGetBoolean) {
    global.loadTimeData.__ntpStorybookOriginalGetBoolean =
      global.loadTimeData.getBoolean.bind(global.loadTimeData)
  }

  global.loadTimeData.getBoolean = (key: string) => {
    if (key === 'featureFlagBraveNewsFeedV2Enabled') {
      return newsFeedV2FeatureFlagEnabled
    }
    return global.loadTimeData.__ntpStorybookOriginalGetBoolean!(key)
  }
}
