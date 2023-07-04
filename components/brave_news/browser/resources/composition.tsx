import * as React from "react";
import { Elements } from "./model";
import { Channel, Publisher, UserEnabled } from "../../../brave_new_tab_ui/api/brave_news";
import Card from "./card";

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

function EntryCard({ entry, publishers }: { entry: Entry, publishers: { [id: string]: Publisher } }) {
  const name = entry.type === "publisher" ? publishers[entry.id].publisherName : entry.id
  return <Card>
    <h2>{name}</h2>
    <b>Subscribed:</b> {entry.subscribed.toString()}
    <b>Feed count:</b> {entry.count}
  </Card>
}

export default function Composition({
  feed,
  publishers,
  channels
}: {
  publishers: { [id: string]: Publisher },
  channels: Channel[],
  feed: Elements[]
}) {
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

  return <div>
    {Object.entries(overview).map(([, value]) => <EntryCard entry={value} publishers={publishers} />)}
  </div>
}
