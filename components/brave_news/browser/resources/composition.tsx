// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from "react";
import { Elements } from "./model";
import { Channel, Publisher, Signal, UserEnabled } from "../../../brave_new_tab_ui/api/brave_news";
import Card from "./card";
import Flex from "../../../brave_new_tab_ui/components/Flex";
import Dropdown from "@brave/leo/react/dropdown";

interface Entry {
  type: 'publisher' | 'channel' | 'special',
  id: string,
  count: number,
  subscribed: boolean
}

function getStats(feed: Elements[], acc: { [id: string]: Entry }) {
  for (const el of feed) {
    if (el.type === "advert") {
      acc.ads.count++
    } else if (el.type === 'discover') {
      acc.discover.count++;
    } else if (el.type === 'cluster') {
      acc[el.clusterType.id].count++
      getStats(el.elements, acc)
    } else {
      acc[el.article.publisherId].count++
    }
  }
}

function EntryCard({ entry, publishers, signals }: { entry: Entry, publishers: { [id: string]: Publisher }, signals: { [id: string]: Signal } }) {
  const name = entry.type === "publisher" ? publishers[entry.id].publisherName : entry.id
  const signal = signals[entry.id]
  return <Card>
    <h2>{name}</h2>
    <div>
      <b>Subscribed:</b> {entry.subscribed.toString()}
    </div>
    <div><b>Feed occurrences:</b> {entry.count}</div>
    <div><b>Weight:</b> {Math.max(signal?.sourceVisits, signal?.channelVisits)}</div>
    <div>
      <b>Visits</b> ({signal?.visitUrls.length})
      <ul>
        {signal?.visitUrls.map(a => <li key={a}><a href={a}>{a}</a></li>)}
      </ul>
    </div>
  </Card>
}

export default function Composition({
  feed,
  publishers,
  channels,
  signals
}: {
  publishers: { [id: string]: Publisher },
  channels: Channel[],
  feed: Elements[],
  signals: { [id: string]: Signal }
}) {
  const [filterType, setFilterType] = React.useState<'everything' | 'special' | 'channel' | 'publisher'>('everything')
  const [filter, setFilter] = React.useState('')

  const overview: { [id: string]: Entry } = {
    ads: { id: 'ads', type: 'special', count: 0, subscribed: false },
    discover: { id: 'discover', type: 'special', count: 0, subscribed: false }
  }
  for (const publisher of Object.keys(publishers)) {
    overview[publisher] = { type: 'publisher', id: publisher, count: 0, subscribed: publishers[publisher].userEnabledStatus === UserEnabled.ENABLED }
  }

  for (const channel of channels) {
    overview[channel.channelName] = { type: 'channel', id: channel.channelName, count: 0, subscribed: !!channel.subscribedLocales.length }
  }

  getStats(feed, overview)
  const filtered = Object.values(overview)
    .filter(a => filterType === 'everything' || a.type === filterType)
    .filter(a => a.id.includes(filter) || publishers[a.id]?.publisherName.includes(filter))
    .sort((a, b) => b.count - a.count)

  return <Flex gap={4} direction="column">
    <input type="text" value={filter} onChange={e => setFilter(e.target.value)} />
    <Dropdown value={filterType} onChange={e => setFilterType(e.detail.value)}>
      <span slot="label">Show stats for:</span>
      <leo-option>everything</leo-option>
      <leo-option>publisher</leo-option>
      <leo-option>channel</leo-option>
      <leo-option>special</leo-option>
    </Dropdown>
    {filtered.map(entry => <EntryCard entry={entry} signals={signals} publishers={publishers} key={entry.id} />)}
  </Flex>
}
