// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import { useInspectContext } from './context';
import Card from './feed/Card';
import Radio from '@brave/leo/react/radioButton'
import Dropdown from '@brave/leo/react/dropdown';
import Flex from '../../../brave_new_tab_ui/components/Flex'
import { Channel, Publisher } from '../../../../../out/Component/gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';
import { color, effect, radius, spacing } from '@brave/leo/tokens/css';

interface Props {
}

const Container = styled(Flex)`
  width: 800px;
`

const SearchBox = styled.input`
  all: unset;
  flex: 1;
  border: 1px solid ${color.divider.subtle};

  &:hover {
    border-color: ${color.divider.strong};
  }

  &:focus {
    border-color: ${color.divider.interactive};
  }

  border-radius: ${radius[8]};
  box-shadow: ${effect.elevation[3]};
  padding: ${spacing[8]};
  height: 32px;
`

function SignalCards<T>({ items, sort, filter, getName, getKey }: { items: T[], sort: 'name' | 'subscribed' | 'visitWeight', filter: string, getName: (item: T) => string, getKey: (item: T) => string }) {
  const { signals } = useInspectContext()

  const filteredAndSorted = items
    .filter(item => getName(item).toLowerCase().includes(filter))
    .sort((a, b) => {
      if (sort === 'name') return getName(a).localeCompare(getName(b))

      const aSignal = signals[getKey(a)]
      const bSignal = signals[getKey(b)]

      if (sort === 'subscribed') return +bSignal.subscribed - +aSignal.subscribed
      return bSignal.visitWeight - aSignal.visitWeight
    })
  return <>
    {filteredAndSorted.map(a => <Card>
      <b>{getName(a)}</b>
      <br />
      <b>Subscribed:</b> {signals[getKey(a)]?.subscribed.toString()}
      <br />
      <b>Visit Weighting:</b> {signals[getKey(a)]?.visitWeight}
    </Card>)}
  </>
}

const getChannelKey = (channel: Channel) => channel.channelName
const getPublisherKey = (p: Publisher) => p.publisherId
const getPublisherName = (p: Publisher) => p.publisherName

export default function SignalsPage(props: Props) {
  const { publishers, channels } = useInspectContext();
  const [show, setShow] = React.useState<'all' | 'publishers' | 'channels'>('all')
  const [sort, setSort] = React.useState<'name' | 'subscribed' | 'visitWeight'>('visitWeight')
  const [filter, setFilter] = React.useState('')

  return <Container direction='column'>
    <h2>Signals Page</h2>
    <Flex direction='row' gap={8}>
      <Flex direction='column' gap={8}>
        Show only:
        <Radio name='show' value="all" currentValue={show} onChange={e => setShow(e.detail.value)} />
        <Radio name='show' value="publishers" currentValue={show} onChange={e => setShow(e.detail.value)} />
        <Radio name='show' value="channels" currentValue={show} onChange={e => setShow(e.detail.value)} />
      </Flex>
      <SearchBox placeholder='filter...' value={filter} onChange={e => setFilter(e.target.value)} />
      <Dropdown value={sort} onChange={e => setSort(e.detail.value)}>
        <div slot="label">Sort by</div>
        <leo-option>name</leo-option>
        <leo-option>subscribed</leo-option>
        <leo-option>visitWeight</leo-option>
      </Dropdown>
    </Flex>

    <Flex direction='column' gap={8}>
      {show !== 'channels' && <>
        <b>Publishers</b>
        <SignalCards filter={filter} getKey={getPublisherKey} getName={getPublisherName} items={Object.values(publishers)} sort={sort} />
      </>}
      {show !== 'publishers' && <>
        <b>Channels</b>
        <SignalCards filter={filter} getKey={getChannelKey} getName={getChannelKey} items={Object.values(channels)} sort={sort} />
      </>}
    </Flex>
  </Container>
}
