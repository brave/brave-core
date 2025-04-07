/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import {
  NewsSignal,
  NewsFeedSpecifier,
  isNewsPublisherEnabled,
  newsFeedSpecifiersEqual } from '../../models/news'

import { useLocale } from '../context/locale_context'
import { useAppState, useAppActions } from '../context/app_model_context'
import classNames from '$web-common/classnames'

import { style } from './feed_nav.style'

export function FeedNav() {
  const { getString } = useLocale()
  const signals = useAppState((s) => s.newsSignals)
  const channels = useAppState((s) => s.newsChannels)
  const publishers = useAppState((s) => s.newsPublishers)

  const enabledChannels = React.useMemo(() => {
    return Array.from(Object.values(channels))
      .filter((channel) => channel.subscribedLocales.length > 0)
      .sort((a, b) => compareVisitWeight(a.channelName, b.channelName, signals))
  }, [channels, signals])

  const enabledPublishers = React.useMemo(() => {
    return Array.from(Object.values(publishers))
      .filter(isNewsPublisherEnabled)
      .sort((a, b) => compareVisitWeight(a.publisherId, b.publisherId, signals))
  }, [publishers, signals])

  return (
    <div data-css-scope={style.scope}>
      <FeedButton
        text={getString('newsFeedAllTitle')}
        specifier={{ type: 'all' }}
      />
      <FeedButton
        text={getString('newsFeedFollowingTitle')}
        specifier={{ type: 'following' }}
      />
      <FeedGroup
        text={getString('newsFeedChannelsTitle')}
        onAddClick={() => {}}
        items={
          enabledChannels.map((channel) => (
            <FeedButton
              key={channel.channelName}
              text={channel.channelName}
              specifier={{ type: 'channel', channel: channel.channelName }}
            />
          ))
        }
      />
      <FeedGroup
        text={getString('newsFeedPublishersTitle')}
        onAddClick={() => {}}
        items={
          enabledPublishers.map((publisher) => (
            <FeedButton
              key={publisher.publisherId}
              text={publisher.publisherName}
              specifier={{
                type: 'publisher',
                publisher: publisher.publisherId
              }}
            />
          ))
        }
      />
    </div>
  )
}

interface FeedButtonProps {
  text: string
  specifier: NewsFeedSpecifier
}

function FeedButton(props: FeedButtonProps) {
  const actions = useAppActions()
  const currentFeed = useAppState((s) => s.currentNewsFeed)
  return (
    <button
      onClick={() => actions.setCurrentNewsFeed(props.specifier)}
      className={classNames({
        selected: newsFeedSpecifiersEqual(currentFeed, props.specifier)
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
  signals: Record<string, NewsSignal>
) {
  if (!signals[1] || !signals[b]) {
    return 0
  }
  return signals[a].visitWeight - signals[b].visitWeight
}
