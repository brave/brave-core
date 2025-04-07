/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import { useLocale } from '../context/locale_context'
import { useAppState, useAppActions } from '../context/app_model_context'
import { FeedNav } from './feed_nav'
import { NewsFeedItem } from '../../models/news'
import { ArticleCard } from './article_card'
import { ClusterCard } from './cluster_card'
import { DiscoverCard } from './discover_card'
import { FeedErrorCard } from './feed_error_card'

import { style } from './news_feed.style'

const itemPageSize = 25

export function NewsFeed() {
  const { getString } = useLocale()
  const actions = useAppActions()

  const newsEnabled = useAppState((s) => s.newsEnabled)
  const feedError = useAppState((s) => s.newsFeedError)
  const feedItems = useAppState((s) => s.newsFeedItems)
  const updateAvailable = useAppState((s) => s.newsUpdateAvailable)

  const [displayCount, setDisplayCount] = React.useState(itemPageSize)

  const loadMore = React.useCallback(() => {
    setDisplayCount((count) => count + itemPageSize)
  }, [])

  if (!newsEnabled) {
    // TODO: Opt-in card?
    return null
  }

  function renderCardContent(item: NewsFeedItem) {
    switch (item.type) {
      case 'article': return <ArticleCard item={item} />
      case 'hero': return <ArticleCard item={item} isHero />
      case 'cluster': return <ClusterCard clusterItem={item} />
      case 'discover': return <DiscoverCard publisherIds={item.publisherIds} />
    }
  }

  function renderFeedItems() {
    if (!feedItems) {
      return <Loading />
    }
    if (feedError) {
      return <FeedErrorCard error={feedError} />
    }
    return <>
      {
        feedItems.slice(0, displayCount).map((item, i) => (
          <div className='feed-card' key={i}>
            {renderCardContent(item)}
          </div>
        ))
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

  return (
    <div data-css-scope={style.scope} data-theme='dark'>
      <VisibilityTracker onVisible={() => actions.onNewsVisible()} />
      {
        updateAvailable && feedItems &&
          <div className='update-available hidden-above-fold'>
            <Button onClick={() => actions.updateNewsFeed()}>
              {getString('newsContentAvailableButtonLabel')}
            </Button>
          </div>
      }
      <div className='sidebar'>
        <FeedNav />
      </div>
      <div className='feed-items'>
        {renderFeedItems()}
      </div>
      <div className='controls-container'>
        <div className='controls hidden-above-fold'>
          <Button fab kind='outline'>
            <Icon name='tune' />
          </Button>
          <Button
            fab
            kind='outline'
            isLoading={!feedItems}
            onClick={() => actions.updateNewsFeed()}
          >
            <Icon name='refresh' />
          </Button>
        </div>
      </div>
    </div>
  )
}

function Loading() {
  const ref = React.useRef<HTMLDivElement>(null)

  // Ensure that when the loading card is removed, then scroll position is not
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

interface VisibilityTrackerProps {
  onVisible: () => void
  rootMargin?: string
}

function VisibilityTracker(props: VisibilityTrackerProps) {
  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (!ref.current) {
      return
    }

    const observer = new IntersectionObserver((entries) => {
      for (const entry of entries) {
        if (entry.intersectionRatio > 0) {
          props.onVisible()
          return
        }
      }
    }, { rootMargin: props.rootMargin || '0px 0px 0px 0px' })

    observer.observe(ref.current)
    return () => { observer.disconnect() }
  }, [props.onVisible, props.rootMargin])

  return <div ref={ref} />
}
