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
import { FeedItemList } from './feed_item_list'
import { NewsOptIn } from './news_opt_in'
import {
  NewsSettingsModal,
  NewsSettingsView,
} from './settings/news_settings_modal'
import { Popover } from '../common/popover'
import { VisibilityTracker } from '../common/visibility_tracker'
import { addCustomEventListener } from '../../lib/custom_event'

import { style } from './news_feed.style'

interface Props {
  standalone?: boolean
}

export function NewsFeed(props: Props) {
  const actions = useNewsActions()

  const newsFeatureEnabled = useNewsState((s) => s.newsFeatureEnabled)
  const newsInitialized = useNewsState((s) => s.initialized)
  const isOptedIn = useNewsState((s) => s.isOptedIn)
  const showOnNTP = useNewsState((s) => s.showOnNTP)
  const feedError = useNewsState((s) => s.feedError)
  const feedItems = useNewsState((s) => s.feedItems)
  const updateAvailable = useNewsState((s) => s.feedUpdateAvailable)

  const [feedVisible, setFeedVisible] = React.useState(false)
  const [navPopoverOpen, setNavPopoverOpen] = React.useState(false)
  const [settingsView, setSettingsView] =
    React.useState<NewsSettingsView | null>(null)

  React.useEffect(() => {
    const listener = () => {
      if (document.visibilityState === 'visible') {
        actions.updateFeed()
      }
    }
    document.addEventListener('visibilitychange', listener)
    return () => document.removeEventListener('visibilitychange', listener)
  }, [])

  React.useEffect(() => {
    // Allow opening the feed settings modal from anywhere on the page.
    return addCustomEventListener('ntp-open-news-feed-settings', () => {
      if (isOptedIn) {
        setSettingsView('default')
      }
    })
  }, [isOptedIn])

  if (!newsFeatureEnabled || !newsInitialized) {
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
        onAddChannelClick={() => setSettingsView('default')}
        onAddPublisherClick={() => setSettingsView('popular')}
        onFeedSelect={() => setNavPopoverOpen(false)}
      />
    )
  }

  return (
    <>
      <FeedContainer standalone={props.standalone}>
        <VisibilityTracker
          onVisible={() => {
            setFeedVisible(true)
            actions.onNewsVisible()
            actions.onInteractionSessionStarted()
          }}
        />
        {updateAvailable && feedItems && (
          <div className='update-available hidden-above-fold'>
            <Button onClick={() => actions.updateFeed({ force: true })}>
              {getString(S.BRAVE_NEWS_NEW_CONTENT_AVAILABLE)}
            </Button>
          </div>
        )}
        <div className='news-feed'>
          <div className='sidebar'>
            <div className='side-nav'>{renderFeedNav()}</div>
          </div>
          <FeedItemList
            feedItems={feedVisible ? feedItems : []}
            feedError={feedError}
          />
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
                onClick={() => setSettingsView('default')}
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
      {settingsView && (
        <NewsSettingsModal
          initialView={settingsView}
          onClose={() => setSettingsView(null)}
        />
      )}
    </>
  )
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
