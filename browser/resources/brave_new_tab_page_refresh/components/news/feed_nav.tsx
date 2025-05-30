/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import {
  Signal,
  FeedV2Type,
  feedType,
  isNewsPublisherEnabled,
  isNewsChannelEnabled,
  newsFeedTypesEqual } from '../../state/news_state'

import { useNewsState, useNewsActions } from '../../context/news_context'
import { getString } from '../../lib/strings'
import classNames from '$web-common/classnames'

import { style } from './feed_nav.style'

interface Props {
  onAddChannelClick: () => void
  onAddPublisherClick: () => void
  onFeedSelect?: () => void
}

export function FeedNav(props: Props) {
  const signals = useNewsState((s) => s.signals)
  const channels = useNewsState((s) => s.channels)
  const publishers = useNewsState((s) => s.publishers)

  const enabledChannels = React.useMemo(() => {
    return Object.values(channels)
      .filter(isNewsChannelEnabled)
      .sort((a, b) => compareVisitWeight(a.channelName, b.channelName, signals))
  }, [channels, signals])

  const enabledPublishers = React.useMemo(() => {
    return Object.values(publishers)
      .filter(isNewsPublisherEnabled)
      .sort((a, b) => compareVisitWeight(a.publisherId, b.publisherId, signals))
  }, [publishers, signals])

  return (
    <div data-css-scope={style.scope}>
      <FeedButton
        text={getString('newsFeedAllTitle')}
        feedType={feedType('all')}
        onFeedSelect={props.onFeedSelect}
      />
      <FeedButton
        text={getString('newsFeedFollowingTitle')}
        feedType={feedType('following')}
        onFeedSelect={props.onFeedSelect}
      />
      <FeedGroup
        text={getString('newsFeedChannelsTitle')}
        onAddClick={() => props.onAddChannelClick()}
        items={
          enabledChannels.map((channel) => (
            <FeedButton
              key={channel.channelName}
              text={channel.channelName}
              onFeedSelect={props.onFeedSelect}
              feedType={feedType('channel', channel.channelName)}
            />
          ))
        }
      />
      <FeedGroup
        text={getString('newsFeedPublishersTitle')}
        onAddClick={() => props.onAddPublisherClick()}
        items={
          enabledPublishers.map((publisher) => (
            <FeedButton
              key={publisher.publisherId}
              text={publisher.publisherName}
              onFeedSelect={props.onFeedSelect}
              feedType={feedType('publisher', publisher.publisherId)}
            />
          ))
        }
      />
    </div>
  )
}

interface FeedButtonProps {
  text: string
  feedType: FeedV2Type
  onFeedSelect?: () => void
}

function FeedButton(props: FeedButtonProps) {
  const actions = useNewsActions()
  const feedType = useNewsState((s) => s.feedType)
  return (
    <button
      onClick={() => {
        actions.setFeedType(props.feedType)
        actions.onSidebarFilterUsage()
        if (props.onFeedSelect) {
          props.onFeedSelect()
        }
      }}
      className={classNames({
        selected: newsFeedTypesEqual(feedType, props.feedType)
      })}
    >
      {props.text}
    </button>
  )
}

const groupDisplayCount = 4

interface FeedGroupProps {
  text: string
  items: React.ReactNode[]
  onAddClick: () => void
}

function FeedGroup(props: FeedGroupProps) {
  const [showAll, setShowAll] = React.useState(false)
  let items = props.items
  if (!showAll) {
    items = items.slice(0, groupDisplayCount)
  }
  return (
    <details
      open
      className={classNames({
        'empty': props.items.length === 0,
        'show-all': showAll
      })}
    >
      <summary>
        <Icon name='arrow-small-right' />
        {props.text}
        <Button
          size='tiny'
          fab
          kind='outline'
          onClick={(event) => {
            event.preventDefault()
            props.onAddClick()
          }}
        >
          <Icon name='plus-add' />
        </Button>
      </summary>
      <div className='group-items'>
        {items}
        {
          props.items.length > groupDisplayCount &&
            <button
              className='show-all-button'
              onClick={() => setShowAll(!showAll)}
            >
              {showAll ? 'Show less' : 'Show more'}
            </button>
        }
      </div>
    </details>
  )
}

function compareVisitWeight(
  a: string,
  b: string,
  signals: Record<string, Signal>
) {
  if (!signals[1] || !signals[b]) {
    return 0
  }
  return signals[a].visitWeight - signals[b].visitWeight
}
