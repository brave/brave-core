/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import Feed from '../../../../../components/brave_news/browser/resources/Feed'
import Variables from '../../../../../components/brave_news/browser/resources/Variables'

import { getString } from '../../lib/strings'
import { FeedNav } from './feed_nav'
import { NewsOptIn } from './news_opt_in'
import { NewsSettingsModal, NewsSettingsView } from './settings/news_settings_modal'
import { Popover } from '../common/popover'
import { VisibilityTracker } from '../common/visibility_tracker'
import { addCustomEventListener } from '../../lib/custom_event'

import { style } from './news_feed.style'

interface Props {
  standalone?: boolean
}

export function NewsFeed(props: Props) {
  const braveNews = useBraveNews()
  const isOptedIn = braveNews.isOptInPrefEnabled
  const showOnNTP = braveNews.isShowOnNTPPrefEnabled
  const feed = braveNews.feedV2
  const updateAvailable = braveNews.feedV2UpdatesAvailable ?? false

  const [navPopoverOpen, setNavPopoverOpen] = React.useState(false)
  const [settingsView, setSettingsView] =
    React.useState<NewsSettingsView | null>(null)

  React.useEffect(() => {
    // Allow opening the feed settings modal from anywhere on the page.
    return addCustomEventListener('ntp-open-news-feed-settings', () => {
      if (isOptedIn) {
        setSettingsView('default')
      }
    })
  }, [isOptedIn])

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

  return <>
    <FeedContainer standalone={props.standalone}>
      <VisibilityTracker
        onVisible={() => braveNews.reportSessionStart()}
      />
      {
        updateAvailable && feed &&
          <div className='update-available hidden-above-fold'>
            <Button onClick={() => braveNews.refreshFeedV2()}>
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
        <div className='feed-list'>
          <Feed
            feed={feed}
            onSessionStart={braveNews.reportSessionStart}
            onViewCountChange={braveNews.reportViewCount}
          />
        </div>
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
              isLoading={!feed}
              onClick={() => braveNews.refreshFeedV2()}
            >
              <Icon name='refresh' />
            </Button>
          </div>
        </div>
      </div>
    </FeedContainer>
    {
      settingsView &&
        <NewsSettingsModal
          initialView={settingsView}
          onClose={() => setSettingsView(null)}
        />
    }
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
      <Variables>
        {props.children}
      </Variables>
    </div>
  )
}
