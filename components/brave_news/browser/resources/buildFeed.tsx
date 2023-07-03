// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  Channel,
  FeedItemMetadata,
  Publisher,
  Signal
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import { normal, pickWeighted, project, tossCoin } from './pick'
import { ArticleElements, Elements } from './model'

export interface FeedSettings {
  blockMinInline: number
  blockMaxInline: number

  inlineDiscoveryRatio: number

  specialCardEveryN: number

  adsToDiscoverRatio: number

  sourceSubscribedMin: number
  sourceSubscribedMax: number

  channelSubscribedMax: number
  channelSubscribedMin: number
  channelVisitsMin: number
  channelVisitsMax: number

  sourcesVisitsMin: number
}

export const defaultFeedSettings = {
  blockMinInline: 1,
  blockMaxInline: 5,
  inlineDiscoveryRatio: 0.25,
  specialCardEveryN: 2,
  adsToDiscoverRatio: 0.5,
  sourceSubscribedMin: 1 / 1e5,
  sourceSubscribedMax: 1,
  channelSubscribedMin: 0.01,
  channelSubscribedMax: 1,
  channelVisitsMin: 0.5,
  channelVisitsMax: 1,
  sourcesVisitsMin: 0.2
}

export const loadSetting = (name: keyof FeedSettings) => {
  const saved = parseFloat(localStorage.getItem(name) || '')
  return isNaN(saved) ? defaultFeedSettings[name] : saved
}

const getSettings = (): FeedSettings => {
  const result: FeedSettings = {} as any
  for (const key of Object.keys(defaultFeedSettings)) {
    result[key] = loadSetting(key as keyof FeedSettings)
  }
  return result
}

const settings = getSettings()
console.log(settings)

export interface Info {
  publishers: { [id: string]: Publisher }
  suggested: Publisher[]
  articles: FeedItemMetadata[]
  signals: { [key: string]: Signal }
  channels: Channel[]
}

export const unvisited = (articles: FeedItemMetadata[], info: Info) => {
  return articles.filter((a) => {
    const signal = info.signals[a.url.url]

    // No signal means we haven't visited the article
    if (!signal) return true

    // No source visits!
    return signal.sourceVisits === 0
  })
}

export const getWeight = (article: FeedItemMetadata, { signals }: Info) => {
  const signal = signals[article.url.url]
  if (!signal) return 0

  if (signal.blocked) return 0

  const channelSubscribedWeighting = signal.channelSubscribed
    ? settings.channelSubscribedMax
    : settings.channelSubscribedMin

  return (
    (settings.sourcesVisitsMin +
      signal.sourceVisits * (1 - settings.sourcesVisitsMin)) *
    signal.popRecency *
    (signal.sourceSubscribed
      ? settings.sourceSubscribedMax
      : settings.sourceSubscribedMin) *
    channelSubscribedWeighting
  )
}

export const getChannelWeight = (channel: Channel, { signals }: Info) => {
  const signal = signals[channel.channelName]
  if (!signal) return 0
  if (signal.blocked) return 0

  const subscribedWeighting = signal.channelSubscribed
    ? settings.channelSubscribedMax
    : settings.channelSubscribedMin

  const visitWeighting = project(
    signal.channelVisits,
    settings.channelVisitsMin,
    settings.channelVisitsMax
  )
  return subscribedWeighting * visitWeighting
}

const generateBlock = (
  info: Info,
  block: { type: 'default' } | { type: 'channel'; id: string }
): ArticleElements[] => {
  const articles =
    block.type === 'default'
      ? [...info.articles]
      : info.articles.filter((a) => {
          const publisher = info.publishers[a.publisherId]
          return publisher.locales.flatMap((l) => l.channels).includes(block.id)
        })

  const elements: ArticleElements[] = []
  const count = project(
    normal(),
    settings.blockMinInline,
    settings.blockMaxInline
  )

  if (!articles.length) {
    console.error('No articles for block', block)
    return []
  }

  elements.push({
    article: pickWeighted(articles, (i) => getWeight(i, info)),
    type: 'hero'
  })

  for (let i = 0; i < count && articles.length > 0; ++i) {
    // Note: Only default blocks can have discover cards.
    const discover = tossCoin(settings.inlineDiscoveryRatio)

    // If this is a discover inline card, then we should pick the most popular unvisited article, by popularity
    if (discover) {
      const discoverable = unvisited(articles, info)
      if (discoverable.length) {
        elements.push({
          type: 'inline',
          article: pickWeighted(discoverable, (i) => i.score),
          isDiscover: true
        })
        continue
      }
    }

    elements.push({
      type: 'inline',
      article: pickWeighted(articles, (i) => getWeight(i, info)),
      isDiscover: false
    })
  }

  // Remove the articles, so we don't reuse them.
  for (const element of elements) {
    const index = info.articles.indexOf(element.article)
    info.articles.splice(index, 1)
  }

  return elements
}

export const generateCluster = (
  info: Info,
  channelOrTopic: string
): Elements | undefined => {
  const elements = generateBlock(info, { type: 'channel', id: channelOrTopic })
  if (!elements.length) return

  return {
    type: 'cluster',
    clusterType: { type: 'channel', id: channelOrTopic },
    elements
  }
}

export const generateRandomCluster = (info: Info) => {
  const randomChannel = pickWeighted([...info.channels.filter(c => c.subscribedLocales.length)], (c) =>
    getChannelWeight(c, info)
  )
  if (!randomChannel) return
  return generateCluster(info, randomChannel.channelName)
}

export const generateAd = (
  info: Info,
  iteration: number
): Elements | undefined => {
  if (iteration % settings.specialCardEveryN !== 0) {
    return
  }

  return tossCoin(settings.adsToDiscoverRatio)
    ? { type: 'advert' }
    : {
        type: 'discover',
        publishers: info.suggested.splice(0, 3)
      }
}

export const generateFeed = (info: Info) => {
  // First step is to filter out articles we're never going to show.
  info.articles = info.articles.filter((a) => getWeight(a, info) > 0)

  let currentStep = 1
  let iteration = 0

  const nextStep = {
    1: 2,
    2: 3,
    3: 4,
    4: 5,
    5: 3
  }

  const steps = {
    1: () => generateBlock(info, { type: 'default' }),
    2: () => generateBlock(info, { type: 'channel', id: 'Top News' }),
    3: () => generateBlock(info, { type: 'default' }),
    4: () => generateRandomCluster(info),
    5: () => generateAd(info, iteration)
  }

  const result: Elements[] = []
  while (info.articles.length !== 0) {
    const generated = steps[currentStep]()
    currentStep = nextStep[currentStep]
    iteration++

    const items = Array.isArray(generated) ? generated : [generated]
    result.push(...items.filter((i) => i))
  }
  return result
}
