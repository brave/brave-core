/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'
import { useInspectContext } from '../../../../../components/brave_news/browser/resources/context'
import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import { FeedView } from '../../../../../components/brave_news/browser/resources/shared/useFeedV2'
import { isPublisherEnabled } from '../../../../../components/brave_news/browser/resources/shared/api'
import { getString } from '../../lib/strings'
import classNames from '$web-common/classnames'

import { style } from './feed_nav.style'

interface Props {
  onAddChannelClick: () => void
  onAddPublisherClick: () => void
  onFeedSelect?: () => void
}

export function FeedNav(props: Props) {
  const { signals } = useInspectContext()
  const { channels, publishers } = useBraveNews()

  const enabledChannels = React.useMemo(() => {
    return Object.values(channels)
      .filter((channel) => channel.subscribedLocales.length > 0)
      .sort((a, b) => compareVisitWeight(a.channelName, b.channelName, signals))
  }, [channels, signals])

  const enabledPublishers = React.useMemo(() => {
    return Object.values(publishers)
      .filter(isPublisherEnabled)
      .sort((a, b) => compareVisitWeight(a.publisherId, b.publisherId, signals))
  }, [publishers, signals])

  return (
    <div data-css-scope={style.scope}>
      <FeedButton
        text={getString('newsFeedAllTitle')}
        feedView='all'
        onFeedSelect={props.onFeedSelect}
      />
      <FeedButton
        text={getString('newsFeedFollowingTitle')}
        feedView='following'
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
              feedView={`channels/${channel.channelName}`}
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
              feedView={`publishers/${publisher.publisherId}`}
            />
          ))
        }
      />
    </div>
  )
}

interface FeedButtonProps {
  text: string
  feedView: FeedView
  onFeedSelect?: () => void
}

function FeedButton(props: FeedButtonProps) {
  const braveNews = useBraveNews()
  return (
    <button
      onClick={() => {
        braveNews.setFeedView(props.feedView)
        braveNews.reportSidebarFilterUsage()
        if (props.onFeedSelect) {
          props.onFeedSelect()
        }
      }}
      className={classNames({
        selected: braveNews.feedView === props.feedView
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
  signals: Record<string, mojom.Signal>
) {
  if (!signals[1] || !signals[b]) {
    return 0
  }
  return signals[a].visitWeight - signals[b].visitWeight
}
