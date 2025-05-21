/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import Feed from '../../../../../components/brave_news/browser/resources/Feed'
import FeedNavigation from '../../../../../components/brave_news/browser/resources/FeedNavigation'
import Variables from '../../../../../components/brave_news/browser/resources/Variables'

import { getString } from '../../lib/strings'
import { NewsOptIn } from './news_opt_in'
import { Popover } from '../common/popover'

import { style } from './news_feed.style'

export function NewsFeed() {
  const braveNews = useBraveNews()
  const isOptedIn = braveNews.isOptInPrefEnabled
  const showOnNTP = braveNews.isShowOnNTPPrefEnabled
  const updateAvailable = braveNews.feedV2UpdatesAvailable ?? false
  const feed = braveNews.feedV2

  const [navPopoverOpen, setNavPopoverOpen] = React.useState(false)

  if (!showOnNTP) {
    return null
  }

  if (!isOptedIn) {
    return (
      <FeedContainer>
        <div className='opt-in-container'>
          <div className='opt-in-card'>
            <NewsOptIn />
          </div>
        </div>
      </FeedContainer>
    )
  }

  return <>
    <FeedContainer>
      <div className='news-feed'>
        {
          updateAvailable && feed &&
            <div className='update-available hidden-above-fold'>
              <Button onClick={() => braveNews.refreshFeedV2()}>
                {getString('newsContentAvailableButtonLabel')}
              </Button>
            </div>
        }
        <div className='sidebar'>
          <FeedNavigation />
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
                <FeedNavigation />
              </Popover>
            </div>
            <Button
              fab
              kind='outline'
              onClick={() => braveNews.setCustomizePage('news')}
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
  </>
}

interface FeedContainerProps {
  standalone?: boolean
  children: React.ReactNode
}

function FeedContainer(props: FeedContainerProps) {
  return (
    <div data-css-scope={style.scope} data-theme='dark'>
      <Variables>
        {props.children}
      </Variables>
    </div>
  )
}
