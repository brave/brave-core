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
import { Channel, Publisher, Signal } from '../../../../../out/Component/gen/brave/components/brave_news/common/brave_news.mojom.m';
import styled from 'styled-components';

interface Props {
}

const Container = styled(Flex)`
  width: 800px;
`

function SignalCard({ item, signals }: { item: Channel | Publisher, signals: { [key: string]: Signal } }) {
  const key = 'channelName' in item ? item.channelName : item.publisherId
  const name = 'channelName' in item ? item.channelName : item.publisherName
  return <Card>
    <b>{name}</b>
    <br />
    <b>Subscribed:</b> {signals[key]?.subscribed}
    <br />
    <b>Visit Weighting:</b> {signals[key]?.visitWeight}
  </Card>
}

export default function SignalsPage(props: Props) {
  const { publishers, channels, signals } = useInspectContext();
  const [show, setShow] = React.useState<'all' | 'publishers' | 'channels'>('all')
  const [sort, setSort] = React.useState<'name' | 'subscribed' | 'visitWeight'>('visitWeight')

  const compareSignals = (a: Signal, b: Signal) => {
    if (sort === 'subscribed') return +b.subscribed - +a.subscribed
    if (sort === 'visitWeight') return b.visitWeight - a.visitWeight
    return 0
  }

  return <Container>
    <h2>Signals Page</h2>
    <Flex direction='row' gap={8}>
      <Flex direction='column' justify='space-between' gap={8}>
        Show only:
        <Radio name='show' value="all" currentValue={show} onChange={e => setShow(e.detail.value)} />
        <Radio name='show' value="publishers" currentValue={show} onChange={e => setShow(e.detail.value)} />
        <Radio name='show' value="channels" currentValue={show} onChange={e => setShow(e.detail.value)} />
      </Flex>
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
        {Object.values(publishers).sort((a, b) => {
          if (sort === 'name') return a.publisherName.localeCompare(b.publisherName)
          return compareSignals(signals[a.publisherId], signals[b.publisherId])
        }).map(p => <SignalCard item={p} signals={signals} />)}
      </>}
      {show !== 'publishers' && <>
        <b>Channels</b>
        {Object.values(channels).sort((a, b) => {
          if (sort === 'name') return a.channelName.localeCompare(b.channelName)
          return compareSignals(signals[a.channelName], signals[b.channelName])
        }).map(c => <SignalCard item={c} signals={signals} />)}
      </>}
    </Flex>
  </Container>
}
