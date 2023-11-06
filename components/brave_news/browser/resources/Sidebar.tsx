// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Icon from '@brave/leo/react/icon';
import { effect, font, radius, spacing } from '@brave/leo/tokens/css';
import * as React from 'react';
import styled, { css } from 'styled-components';
import { useInspectContext } from './context';
import Card from './feed/Card';
import { FeedType, useBraveNews } from './shared/Context';
import { isPublisherEnabled } from './shared/api';

const DEFAULT_SHOW_COUNT = 4;

const PAD_LEFT = '34px';

const Container = styled(Card)`
  width: 270px;

  padding: ${spacing.xl} ${spacing.m};
  align-self: flex-start;
  position: sticky;
  top: ${spacing.xl};
`

const Heading = styled.h3`
  font: ${font.primary.default.semibold};
  color: var(--bn-glass-25);
  margin: 0;

  padding-left: ${PAD_LEFT};
`

const CustomButton = styled.button <{ selected?: boolean, faint?: boolean, bold?: boolean }>`
  padding: ${spacing.m};
  padding-left: ${PAD_LEFT};

  display: block;
  outline: none;
  border-radius: ${radius.m};
  border: none;
  background: none;
  text-align: left;
  width: 100%;

  color: ${p => p.faint ? `var(--bn-glass-25)` : `var(--bn-glass-70)`};
  font: ${p => font.primary.small[p.bold ? 'semibold' : 'regular']};
  cursor: pointer;

  &:hover {
    opacity: 0.7;
  }

  &:focus-visible {
    box-shadow: ${effect.focusState};
  }

  ${p => p.selected && css`background: var(--bn-glass-10);`}
`
const Section = styled.details`
  & summary {
    --leo-icon-size: 18px;

    color: var(--bn-glass-100);
    padding: ${spacing.m};
    border-radius: ${radius.m};
    outline: none;

    display: flex;
    align-items: center;
    gap: ${spacing.m};
    list-style: none;
    font: ${font.primary.small.semibold};

    cursor: pointer;

    :focus-visible {
      box-shadow: ${effect.focusState};
    }

    ${CustomButton} {
      padding: 0;
      flex: 0;
      display: flex;
      gap: ${spacing.m};
      align-items: center;
      margin-left: auto;
    }
  }

  & summary .marker {
    display: inline-block;
    transition: transform 0.1s ease-in-out;
    color: var(--bn-card-color);
  }

  &[open] summary .marker {
    transform: rotate(90deg);
  }
`

const Marker = <Icon name='arrow-small-right' className='marker' />

export function Item(props: { id: FeedType, name: string }) {
  const { feedView, setFeedView } = useBraveNews()

  return <CustomButton selected={props.id === feedView} onClick={() => setFeedView(props.id)} bold={props.id === 'all'}>
    {props.name}
  </CustomButton>
}

export default function Sidebar() {
  const  {channels, publishers} = useBraveNews()
  const { signals } = useInspectContext()

  const [showingMoreChannels, setShowingMoreChannels] = React.useState(false)
  const [showingMorePublishers, setShowingMorePublishers] = React.useState(false)

  const slicedPublisherIds = React.useMemo(() => Object.keys(publishers)
    .filter(p => isPublisherEnabled(publishers[p]))
    .sort((a, b) => signals[b]?.visitWeight - signals[a]?.visitWeight)
    .slice(0, showingMorePublishers ? undefined : DEFAULT_SHOW_COUNT), [publishers, showingMorePublishers, signals])
  const slicedChannelIds = React.useMemo(() => Object.keys(channels)
    .filter(c => channels[c].subscribedLocales.length)
    .sort((a, b) => signals[b]?.visitWeight - signals[a]?.visitWeight)
    .slice(0, showingMoreChannels ? undefined : DEFAULT_SHOW_COUNT), [channels, showingMoreChannels, signals])

  return <Container>
    <Heading>My Feed</Heading>
    <Item id='all' name="All" />
    <Section open>
      <summary>
        {Marker}
        Channels
        <CustomButton faint onClick={() => {/* TODO(fallaciousreasoning): When we're on the NTP wire this up */}}>
          <Icon name='plus-add' />
          Add
        </CustomButton>
      </summary>
      {slicedChannelIds.map(c => <Item key={c} id={`channels/${c}`} name={c} />)}
      {!showingMoreChannels && slicedChannelIds.length >= DEFAULT_SHOW_COUNT && <CustomButton faint onClick={() => setShowingMoreChannels(true)}>Show all</CustomButton>}
    </Section>
    <Section open>
      <summary>
        {Marker}
        Publishers
        <CustomButton faint>
          <Icon name='plus-add' onClick={() => {/* TODO(fallaciousreasoning): When we're on the NTP wire this up */}} />
          Add
        </CustomButton>
      </summary>
      {slicedPublisherIds.map(p => <Item key={p} id={`publishers/${p}`} name={publishers[p]?.publisherName} />)}
      {!showingMorePublishers && slicedPublisherIds.length >= DEFAULT_SHOW_COUNT && <CustomButton faint onClick={() => setShowingMorePublishers(true)}>Show all</CustomButton>}
    </Section>
  </Container>
}
