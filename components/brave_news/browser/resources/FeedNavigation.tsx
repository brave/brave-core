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
import { useBraveNews } from './shared/Context';
import { isPublisherEnabled } from './shared/api';
import { FeedView } from './shared/useFeedV2';
import { getLocale } from '$web-common/locale';
import SettingsButton from './SettingsButton';

const DEFAULT_SHOW_COUNT = 4;

const PAD_LEFT = '34px';

const Container = styled(Card)`
  width: 270px;

  padding: ${spacing.xl} ${spacing.m};
  align-self: flex-start;
  position: sticky;
  top: ${spacing.xl};

  max-height: calc(100vh - ${spacing.xl} * 2);
  overflow-y: auto;

  scrollbar-width: thin;
  scrollbar-color: var(--bn-glass-10) var(--bn-glass-10);
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

  color: ${p => p.faint ? `var(--bn-glass-50)` : `var(--bn-glass-100)`};
  font: ${p => font.small[p.bold ? 'semibold' : 'regular']};
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
    font: ${font.small.semibold};

    cursor: pointer;

    :focus-visible {
      box-shadow: ${effect.focusState};
    }

    ${SettingsButton} {
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

const AddButton = styled(SettingsButton)`
  --leo-button-padding: ${spacing.xs};
`

function usePersistedState<T>(name: string, defaultValue: T) {
  const [value, setValue] = React.useState<T>(JSON.parse(localStorage[name] ?? null) ?? defaultValue)
  React.useEffect(() => {
    localStorage[name] = JSON.stringify(value)
  }, [name, value])

  return [value, setValue] as const
}

const Marker = <Icon name='arrow-small-right' className='marker' />
const PlaceholderMarker = <Icon />

export function Item(props: { id: FeedView, name: string }) {
  const { feedView, setFeedView, reportSidebarFilterUsage } = useBraveNews()
  const topLevel = ['all', 'following'].includes(props.id)
  return <CustomButton
    selected={props.id === feedView}
    onClick={() => {
      setFeedView(props.id)
      reportSidebarFilterUsage()
    }}
    bold={topLevel}
  >
    {props.name}
  </CustomButton>
}

export default function Sidebar() {
  const { channels, publishers, setCustomizePage } = useBraveNews()
  const { signals } = useInspectContext()

  const [showingMoreChannels, setShowingMoreChannels] = usePersistedState("showingMoreChannels", false)
  const [showingMorePublishers, setShowingMorePublishers] = usePersistedState("showingMorePublishers", false)

  const subscribedPublisherIds = React.useMemo(() => Object.keys(publishers)
    .filter(p => isPublisherEnabled(publishers[p]))
    .sort((a, b) => signals[b]?.visitWeight - signals[a]?.visitWeight), [publishers, signals])
  const slicedPublisherIds = React.useMemo(() => subscribedPublisherIds
    .slice(0, showingMorePublishers ? undefined : DEFAULT_SHOW_COUNT), [subscribedPublisherIds, showingMorePublishers])
  const subscribedChannels = React.useMemo(() => Object.keys(channels)
    .filter(c => channels[c].subscribedLocales.length)
    .sort((a, b) => signals[b]?.visitWeight - signals[a]?.visitWeight),
    [channels, signals])
  const slicedChannelIds = React.useMemo(() => subscribedChannels
    .slice(0, showingMoreChannels ? undefined : DEFAULT_SHOW_COUNT), [subscribedChannels, showingMoreChannels])

  return <Container>
    <Item id='all' name={getLocale('braveNewsForYouFeed')} />
    <Item id='following' name={getLocale('braveNewsFollowingFeed')} />
    <Section open>
      <summary>
        {subscribedChannels.length ? Marker : PlaceholderMarker}
        {getLocale('braveNewsChannelsHeader')}
        <AddButton size="tiny" onClick={e => {
          setCustomizePage('news')
          e.stopPropagation()
        }}>
          <Icon name='plus-add' />
        </AddButton>
      </summary>
      {slicedChannelIds.map(c => <Item key={c} id={`channels/${c}`} name={c} />)}
      {subscribedChannels.length > DEFAULT_SHOW_COUNT
        && <CustomButton faint onClick={() => setShowingMoreChannels(s => !s)}>
          {showingMoreChannels
            ? getLocale('braveNewsShowLess')
            : getLocale('braveNewsShowAll')}
        </CustomButton>}
    </Section>
    <Section open>
      <summary>
        {subscribedPublisherIds.length ? Marker : PlaceholderMarker}
        {getLocale('braveNewsPublishersHeading')}
        <AddButton size="tiny" onClick={e => {
          setCustomizePage('popular')
          e.stopPropagation()
        }}>
          <Icon name='plus-add' />
        </AddButton>
      </summary>
      {slicedPublisherIds.map(p => <Item key={p} id={`publishers/${p}`} name={publishers[p]?.publisherName} />)}
      {subscribedPublisherIds.length > DEFAULT_SHOW_COUNT
        && <CustomButton faint onClick={() => setShowingMorePublishers(s => !s)}>
          {showingMorePublishers
            ? getLocale('braveNewsShowLess')
            : getLocale('braveNewsShowAll')}
        </CustomButton>}
    </Section>
  </Container>
}
