// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Importing this module registers a mock BraveNewsController on `window` so
// Storybook can render the Brave News frontend without a Mojo backend. It also
// enables the FeedV2 feature flag and opts the configuration in, so the feed
// actually loads. Import it (for its side effects) at the top of a story,
// before anything that calls getBraveNewsController().
import {
  BraveNewsControllerRemote,
  ClusterType,
  FeedItemMetadata,
  FeedItemV2,
  FeedV2,
  Image
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import getBraveNewsController from '../shared/api'
import { ConfigurationCachingWrapper } from '../shared/configurationCache'
import { loadTimeData } from '$web-common/loadTimeData'

// The base::Time epoch (1601) is offset from the Unix epoch (1970) by this many
// milliseconds. mojoTimeToJSDate() applies the inverse to read it back.
const EPOCH_DELTA_MS = Date.UTC(1970, 0, 1) - Date.UTC(1601, 0, 1)
const jsDateToMojoTime = (date: Date) => ({
  internalValue: BigInt((date.getTime() + EPOCH_DELTA_MS) * 1000)
})

const image = (seed: string): Image => ({
  paddedImageUrl: undefined,
  imageUrl: { url: `https://picsum.photos/seed/${seed}/304/176` }
})

let nextId = 0
const metadata = (
  title: string,
  publisherName: string,
  description: string,
  minutesAgo: number
): FeedItemMetadata => {
  const id = nextId++
  return {
    categoryName: 'Top News',
    channels: ['Top News'],
    publishTime: jsDateToMojoTime(new Date(Date.now() - minutesAgo * 60 * 1000)),
    title,
    description,
    url: { url: `https://example.com/article/${id}` },
    urlHash: `${id}`,
    image: image(`bn-${id}`),
    publisherId: `publisher-${id}`,
    publisherName,
    score: 100,
    popScore: 0,
    relativeTimeDescription: `${minutesAgo} minutes ago`
  }
}

const article = (
  title: string,
  publisherName: string,
  description: string,
  minutesAgo: number
): FeedItemV2 => ({
  hero: undefined,
  discover: undefined,
  cluster: undefined,
  article: { isDiscover: false, data: metadata(title, publisherName, description, minutesAgo) }
})

const items: FeedItemV2[] = [
  {
    article: undefined,
    discover: undefined,
    cluster: undefined,
    hero: {
      data: metadata(
        'Brave News comes to the sidebar',
        'Brave',
        'Your personalized feed is now a click away, right inside the side panel.',
        12
      )
    }
  },
  article(
    'Why protocol-relative image URLs matter',
    'The Verge',
    'A small change lets the same components render under chrome:// and chrome-untrusted://.',
    21
  ),
  article(
    'Storybook without a browser process',
    'Smashing Magazine',
    'Mocking Mojo controllers keeps component stories fast and dependency-free.',
    34
  ),
  {
    article: undefined,
    hero: undefined,
    discover: undefined,
    cluster: {
      type: ClusterType.CHANNEL,
      id: 'Technology',
      articles: [
        {
          hero: undefined,
          article: {
            isDiscover: false,
            data: metadata(
              'A cluster groups related stories',
              'Ars Technica',
              'Clusters surface a channel or topic with several articles at once.',
              45
            )
          }
        },
        {
          hero: undefined,
          article: {
            isDiscover: false,
            data: metadata(
              'Second story in the cluster',
              'Wired',
              'More from the same channel, rendered together.',
              48
            )
          }
        }
      ]
    }
  },
  article(
    'Following feed, channel feed, all feed',
    'TechCrunch',
    'The sidebar reuses the same FeedV2 surface as the new tab page.',
    52
  )
]

const feed: FeedV2 = {
  constructTime: jsDateToMojoTime(new Date()),
  sourceHash: 'storybook-mock',
  type: undefined,
  items,
  error: undefined
}

export const mockBraveNewsController: Partial<BraveNewsControllerRemote> = {
  async getLocale() {
    return { locale: 'en_US' }
  },

  async getFeedV2() {
    return { feed }
  },

  async getFollowingFeed() {
    return { feed }
  },

  async getChannelFeed() {
    return { feed }
  },

  async getPublisherFeed() {
    return { feed }
  },

  ensureFeedV2IsUpdating() {},
  async setConfiguration() {},
  onNewCardsViewed() {},
  onCardVisited() {},
  onInteractionSessionStarted() {},
  onSidebarFilterUsage() {}
}

// @ts-expect-error mockBraveNewsController only implements the methods the feed
// frontend calls, not the full BraveNewsControllerRemote interface.
window.storybookBraveNewsController = mockBraveNewsController

// The feed only fetches when the V2 flag is on; the default Storybook
// loadTimeData returns false for every boolean, so enable it here.
const baseGetBoolean = loadTimeData.getBoolean.bind(loadTimeData)
loadTimeData.getBoolean = (key: string) =>
  key === 'featureFlagBraveNewsFeedV2Enabled' ? true : baseGetBoolean(key)

// Make sure the controller singleton resolves to the mock before any listener
// wrappers are constructed, then opt in so the feed is shown.
getBraveNewsController()
ConfigurationCachingWrapper.getInstance().set({ isOptedIn: true, showOnNTP: true })
