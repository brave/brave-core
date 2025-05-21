/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'
import { loadTimeData } from '$web-common/loadTimeData'

function createController(): Partial<mojom.BraveNewsControllerInterface> {
  loadTimeData.getBoolean = (key: string) => {
    if (key === 'featureFlagBraveNewsFeedV2Enabled') {
      return true
    }
    return false
  }

  return {
    async getLocale() { return { locale: 'en-US' } },
    addFeedListener(listener) {},
    addChannelsListener(listener) {},
    addConfigurationListener(listener) {},
    async setConfiguration(configuration) {},
    onInteractionSessionStarted() {},
    ensureFeedV2IsUpdating() {},
    async getFeedV2() {
      return {
        feed: {
          constructTime: { internalValue: BigInt(0) },
          sourceHash: 'a',
          type: {
            all: {},
            following: undefined,
            channel: undefined,
            publisher: undefined
          },
          items: [
            feedItem({
              hero: {
                data: feedItemMetadata({
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
                      data: feedItemMetadata({
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
                      data: feedItemMetadata({
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
          error: undefined
        }
      }
    }
  }
}

function feedItem(data: Partial<mojom.FeedItemV2>) {
  return {
    article: undefined,
    advert: undefined,
    hero: undefined,
    cluster: undefined,
    discover: undefined,
    ...data
  }
}

function feedItemMetadata(
  data: Partial<mojom.FeedItemMetadata>
) : mojom.FeedItemMetadata {
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

Object.assign(window, {
  storybookBraveNewsController: createController()
})
