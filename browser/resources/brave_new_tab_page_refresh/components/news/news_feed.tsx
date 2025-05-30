/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useNewsState, useNewsActions } from '../../context/news_context'
import { getString } from '../../lib/strings'
import { FeedNav } from './feed_nav'
import { NewsOptIn } from './news_opt_in'
import { Popover } from '../common/popover'
import { VisibilityTracker } from '../common/visibility_tracker'

import { style } from './news_feed.style'

interface Props {
  standalone?: boolean
}

export function NewsFeed(props: Props) {
  const actions = useNewsActions()

  const newsInitializing = useNewsState((s) => s.newsInitializing)
  const isOptedIn = useNewsState((s) => s.isOptedIn)
  const showOnNTP = useNewsState((s) => s.showOnNTP)
  const feedItems = useNewsState((s) => s.feedItems)
  const updateAvailable = useNewsState((s) => s.feedUpdateAvailable)

  const [navPopoverOpen, setNavPopoverOpen] = React.useState(false)

  React.useEffect(() => {
    const listener = () => {
      if (document.visibilityState === 'visible') {
        actions.updateFeed()
      }
    }
    document.addEventListener('visibilitychange', listener)
    return () => document.removeEventListener('visibilitychange', listener)
  }, [])

  if (newsInitializing) {
    return null
  }

  if (!showOnNTP && !props.standalone) {
    return null
  }

  if (!isOptedIn) {
    return (
      <FeedContainer standalone={props.standalone}>
        <div className='opt-in-container'>
          <div className='opt-in-card'>
            <NewsOptIn />
          </div>
        </div>
      </FeedContainer>
    )
  }

  function renderFeedNav() {
    return (
      <FeedNav
        onAddChannelClick={() => {}}
        onAddPublisherClick={() => {}}
        onFeedSelect={() => setNavPopoverOpen(false)}
      />
    )
  }

  return <>
    <FeedContainer standalone={props.standalone}>
      <VisibilityTracker
        onVisible={() => {
          actions.onNewsVisible()
          actions.onInteractionSessionStarted()
        }}
      />
      {
        updateAvailable && feedItems &&
          <div className='update-available hidden-above-fold'>
            <Button onClick={() => actions.updateFeed({ force: true })}>
              {getString('newsContentAvailableButtonLabel')}
            </Button>
          </div>
      }
      <div className='news-feed'>
        <div className='sidebar'>
          <div className='side-nav'>
            {renderFeedNav()}
          </div>
        </div>
        <div className='feed-item-list' />
        <div className='controls-container'>
          <div className='controls hidden-above-fold'>
            <div className='popover-nav-control'>
              <Button
                fab
                kind='outline'
                className='show-nav-button'
                onClick={() => {
                  setNavPopoverOpen(!navPopoverOpen)
                }}
              >
                <Icon name='hamburger-menu' />
              </Button>
              <Popover
                className='popover-nav'
                isOpen={navPopoverOpen}
                onClose={() => setNavPopoverOpen(false)}
              >
                {renderFeedNav()}
              </Popover>
            </div>
            <Button
              fab
              kind='outline'
              onClick={() => {}}
            >
              <Icon name='tune' />
            </Button>
            <Button
              fab
              kind='outline'
              isLoading={!feedItems}
              onClick={() => actions.updateFeed({ force: true })}
            >
              <Icon name='refresh' />
            </Button>
          </div>
        </div>
      </div>
    </FeedContainer>
  </>
}

interface FeedContainerProps {
  standalone?: boolean
  children: React.ReactNode
}

function FeedContainer(props: FeedContainerProps) {
  return (
    <div
      data-css-scope={style.scope}
      data-theme='dark'
      className={props.standalone ? 'standalone' : ''}
    >
      {props.children}
    </div>
  )
}
