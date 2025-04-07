/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import { useLocale } from '../context/locale_context'
import { useNewsState, useNewsActions } from '../context/news_context'
import { FeedNav } from './feed_nav'
import { FeedItemV2 } from '../../api/news'
import { ArticleCard } from './article_card'
import { ClusterCard } from './cluster_card'
import { DiscoverCard } from './discover_card'
import { FeedErrorCard } from './feed_error_card'
import { NewsSettingsModal, NewsSettingsView } from './settings/news_settings_modal'
import { Popover } from '../common/popover'
import { VisibilityTracker } from '../common/visibility_tracker'

import { style } from './news_feed.style'

const itemPageSize = 25

interface Props {
  standalone?: boolean
}

export function NewsFeed(props: Props) {
  const { getString } = useLocale()
  const actions = useNewsActions()

  const newsInitializing = useNewsState((s) => s.newsInitializing)
  const newsEnabled = useNewsState((s) => s.newsEnabled)
  const feedError = useNewsState((s) => s.newsFeedError)
  const feedItems = useNewsState((s) => s.newsFeedItems)
  const updateAvailable = useNewsState((s) => s.newsUpdateAvailable)

  const [displayCount, setDisplayCount] = React.useState(itemPageSize)
  const [navPopoverOpen, setNavPopoverOpen] = React.useState(false)
  const [settingsView, setSettingsView] =
    React.useState<NewsSettingsView | null>(null)

  const loadMore = React.useCallback(() => {
    setDisplayCount((count) => count + itemPageSize)
  }, [])

  React.useEffect(() => {
    const listener = () => {
      if (document.visibilityState === 'visible') {
        actions.updateNewsFeed()
      }
    }
    document.addEventListener('visibilitychange', listener)
    return () => document.removeEventListener('visibilitychange', listener)
  }, [])

  if (!newsEnabled) {
    return null
  }

  let articleIndex = 0

  function getArticleIndex(step: number) {
    const index = articleIndex
    articleIndex += step
    return index
  }

  function renderCardContent(item: FeedItemV2) {
    if (item.article) {
      return (
        <ArticleCard
          item={item.article.data}
          articleIndex={getArticleIndex(1)}
        />
      )
    }
    if (item.hero) {
      return (
        <ArticleCard
          item={item.hero.data}
          articleIndex={getArticleIndex(1)}
        />
      )
    }
    if (item.cluster) {
      return (
        <ClusterCard
          clusterItem={item.cluster}
          articleIndex={getArticleIndex(item.cluster.articles.length)}
        />
      )
    }
    if (item.discover) {
      return <DiscoverCard publisherIds={item.discover.publisherIds} />
    }
    return null
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

  function renderFeedItems() {
    if (!feedItems) {
      return <Loading />
    }
    if (feedError.hasValue()) {
      return <FeedErrorCard error={feedError.value()} />
    }
    return <>
      {
        feedItems.slice(0, displayCount).map((item, i) => {
          const content = renderCardContent(item)
          return content
            ? <div className='feed-card' key={i}>{content}</div>
            : null
        })
      }
      {
        feedItems.length > displayCount ?
          <VisibilityTracker
            rootMargin='0px 0px 1000px 0px'
            onVisible={loadMore}
          /> :
          <div className='caught-up'>
            <hr />
            <p>
              <Icon name='check-circle-outline' />
              {getString('newsCaughtUpText')}
            </p>
            <hr />
          </div>
      }
    </>
  }

  function renderControls() {
    return (
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
          onClick={() => actions.updateNewsFeed({ force: true })}
        >
          <Icon name='refresh' />
        </Button>
      </div>
    )
  }

  return <>
    <div
      data-css-scope={style.scope}
      data-theme='dark'
      className={props.standalone ? 'standalone' : ''}
    >
      <VisibilityTracker
        onVisible={() => {
          actions.onNewsVisible()
          actions.notifyNewsInteractionSessionStarted()
        }}
      />
      {
        updateAvailable && feedItems &&
          <div className='update-available hidden-above-fold'>
            <Button onClick={() => actions.updateNewsFeed({ force: true })}>
              {getString('newsContentAvailableButtonLabel')}
            </Button>
          </div>
      }
      <div className='news-feed'>
        <div className='sidebar'>
          {
            !newsInitializing &&
              <div className='side-nav'>
                {renderFeedNav()}
              </div>
          }
        </div>
        <div className='feed-items'>
          {renderFeedItems()}
        </div>
        <div className='controls-container'>
          {renderControls()}
        </div>
      </div>
    </div>
    {
      settingsView &&
        <NewsSettingsModal
          initialView={settingsView}
          onClose={() => setSettingsView(null)}
        />
    }
  </>
}

function Loading() {
  const ref = React.useRef<HTMLDivElement>(null)

  // Ensure that when the loading card is removed, scroll position is not
  // restored further down the page.
  React.useEffect(() => {
    const parent = ref.current?.closest('[data-css-scope]')
    if (!parent) {
      return
    }
    const rect = parent.getBoundingClientRect()
    const top = rect.y + window.scrollY
    return () => {
      if (top < window.scrollY) {
        window.scrollTo({ top })
      }
    }
  }, [])

  return (
    <div ref={ref} className='feed-card loading'>
      <ProgressRing />
    </div>
  )
}
