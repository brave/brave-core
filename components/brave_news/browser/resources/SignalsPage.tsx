// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import { useInspectContext } from './context';
import Card from './feed/Card';
import Radio from '@brave/leo/react/radioButton'
import Button from '@brave/leo/react/button'
import Dropdown from '@brave/leo/react/dropdown';
import Input from '@brave/leo/react/input';
import Flex from '$web-common/Flex'
import { Channel, Publisher } from 'gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';
import FeedStats, { getFeedStats } from './FeedStats';
import { useBraveNews } from './shared/Context';

interface Props {
}

const Container = styled(Flex)`
`

function SignalCards<T>({ items, sort, filter, getName, getKey, stats }: { items: T[], sort: 'name' | 'subscribed' | 'visitWeight' | 'shownCount', filter: string, getName: (item: T) => string, getKey: (item: T) => string, stats: { [key: string]: number } }) {
  const { signals } = useInspectContext()

  const filteredAndSorted = items
    .filter(item => getName(item).toLowerCase().includes(filter))
    .sort((a, b) => {
      const aSignal = signals[getKey(a)]
      const bSignal = signals[getKey(b)]

      if (sort === 'name' || !aSignal || !bSignal) return getName(a).localeCompare(getName(b))

      if (sort === 'shownCount') {
        const aCount = stats[getKey(a)] ?? 0
        const bCount = stats[getKey(b)] ?? 0
        return bCount - aCount
      }
      if (sort === 'subscribed') return bSignal.subscribedWeight - aSignal.subscribedWeight
      return bSignal.visitWeight - aSignal.visitWeight
    })
  return <>
    {filteredAndSorted.map(a => <Card>
      <b>{getName(a)}</b>
      <br />
      <b>Subscribed Weighting:</b> {signals[getKey(a)]?.subscribedWeight.toString()}
      <br />
      <b>Visit Weighting:</b> {signals[getKey(a)]?.visitWeight}
      <br />
      <b>Shown count:</b> {stats[getKey(a)] ?? 0}
    </Card>)}
  </>
}

const getChannelKey = (channel: Channel) => channel.channelName
const getPublisherKey = (p: Publisher) => p.publisherId
const getPublisherName = (p: Publisher) => p.publisherName

export default function SignalsPage(props: Props) {
  const { feed, truncate, setTruncate } = useInspectContext();
  const { channels, publishers } = useBraveNews();

  const [show, setShow] = React.useState<'all' | 'publishers' | 'channels'>('all')
  const [sort, setSort] = React.useState<'name' | 'subscribed' | 'visitWeight' | 'shownCount'>('visitWeight')
  const [filter, setFilter] = React.useState('')
  const { channelStats, publisherStats, counts } = getFeedStats(feed, truncate)

  return <Container direction='column'>
    <h2>Signals</h2>
    <Flex direction='row' gap={8} wrap='wrap'>
      <div>
        Publishers: {Object.keys(publishers).length}
        <br />
        Channels: {Object.keys(channels).length}
        <br />
        <Button onClick={() => window.location.reload()}>Refresh</Button>
      </div>
      <Flex direction='column' gap={8}>
        Show only:
        <Radio name='show' value="all" currentValue={show} onChange={e => setShow(e.detail.value)} />
        <Radio name='show' value="publishers" currentValue={show} onChange={e => setShow(e.detail.value)} />
        <Radio name='show' value="channels" currentValue={show} onChange={e => setShow(e.detail.value)} />
      </Flex>
      <Input placeholder='filter...' value={filter} onInput={e => setFilter(e.detail.value)}>
        Filter
      </Input>
      <Dropdown value={sort} onChange={e => setSort(e.detail.value)}>
        <div slot="label">Sort by</div>
        <leo-option>name</leo-option>
        <leo-option>subscribed</leo-option>
        <leo-option>visitWeight</leo-option>
        <leo-option>shownCount</leo-option>
      </Dropdown>
      <Input type="text" hasErrors={false} showErrors={false} mode='outline' size='normal' value={truncate} onChange={e => setTruncate(parseInt((e.detail.value)))}>
        Consider first {"{n}"} cards
      </Input>
    </Flex>

    <Flex direction='column' gap={8}>
      <b>Stats:</b>
      <FeedStats {...counts} />
    </Flex>

    <Flex direction='column' gap={8}>
      {show !== 'channels' && <>
        <b>Publishers</b>
        <SignalCards filter={filter} getKey={getPublisherKey} getName={getPublisherName} items={Object.values(publishers)} sort={sort} stats={publisherStats} />
      </>}
      {show !== 'publishers' && <>
        <b>Channels</b>
        <SignalCards filter={filter} getKey={getChannelKey} getName={getChannelKey} items={Object.values(channels)} sort={sort} stats={channelStats} />
      </>}
    </Flex>
  </Container>
}
