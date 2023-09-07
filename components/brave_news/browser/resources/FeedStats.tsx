// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import Card from './feed/Card';
import { FeedV2 } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';

interface Counts {
  advert: number,
  discover: number,
  cluster: number,
  article: number,
  inlineDiscover: number,
  hero: number
}

export const getFeedStats = (feed: FeedV2 | undefined, truncate?: number) => {
  const counts = {
    advert: 0,
    discover: 0,
    cluster: 0,
    article: 0,
    inlineDiscover: 0,
    hero: 0
  }
  const channelStats: { [key: string]: number } = {}
  const publisherStats: { [key: string]: number } = {}

  if (!feed) {
    return { counts, channelStats, publisherStats }
  }

  const copy = feed.items.slice(0, truncate)
  for (const item of copy) {
    for (const type of Object.keys(item)) counts[type]++

    const article = item.article || item.hero
    if (article) {
      if (!channelStats[article.data.categoryName]) channelStats[article.data.categoryName] = 0
      if (!publisherStats[article.data.publisherId]) publisherStats[article.data.publisherId] = 0
      channelStats[article.data.categoryName]++
      publisherStats[article.data.publisherId]++
    }

    if (item.article?.isDiscover)
      counts.inlineDiscover++
  }

  return {
    counts,
    channelStats,
    publisherStats
  }
}

export default function FeedStats(props: Counts) {
  return <Card>
    <b>Adverts: </b> {props.advert}
    <div><b>Discover Publishers: </b> {props.discover}</div>
    <div><b>Heros: </b> {props.hero}</div>
    <div><b>Articles: </b> {props.article}</div>
    <div><b>Clusters: </b> {props.cluster}</div>
    <div><b>Discover/Subscribed Article Ratio: </b> {Math.round(props.inlineDiscover * 100 / props.article) / 100}</div>
  </Card>
}
