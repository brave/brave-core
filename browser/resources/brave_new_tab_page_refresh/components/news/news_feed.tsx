/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import { FeedItemV2, FeedV2Error } from '../../api/news_api'
import { useNewsState, useNewsActions } from '../../context/news_context'
import { useLocale } from '../../context/locale_context'
import { FeedNav } from './feed_nav'
import { ArticleCard } from './article_card'
import { ClusterCard } from './cluster_card'
import { DiscoverCard } from './discover_card'
import { FeedErrorCard } from './feed_error_card'
import { NewsOptIn } from './news_opt_in'
import { NewsSettingsModal, NewsSettingsView } from './settings/news_settings_modal'
import { Popover } from '../common/popover'
import { VisibilityTracker } from '../common/visibility_tracker'
import { Optional } from '../../lib/optional'
import { addCustomEventListener } from '../../lib/custom_event'

import { style } from './news_feed.style'

const itemPageSize = 25

interface Props {
  standalone?: boolean
}

export function NewsFeed(props: Props) {
  const { getString } = useLocale()
  const actions = useNewsActions()

  const newsInitializing = useNewsState((s) => s.newsInitializing)
  const showNewsFeed = useNewsState((s) => s.showNewsFeed)
  const newsEnabled = useNewsState((s) => s.newsEnabled)
  const feedError = useNewsState((s) => s.newsFeedError)
  const feedItems = useNewsState((s) => s.newsFeedItems)
  const updateAvailable = useNewsState((s) => s.newsUpdateAvailable)

  const [navPopoverOpen, setNavPopoverOpen] = React.useState(false)
  const [settingsView, setSettingsView] =
    React.useState<NewsSettingsView | null>(null)

  React.useEffect(() => {
    const listener = () => {
      if (document.visibilityState === 'visible') {
        actions.updateNewsFeed()
      }
    }
    document.addEventListener('visibilitychange', listener)
    return () => document.removeEventListener('visibilitychange', listener)
  }, [])

  React.useEffect(() => {
    return addCustomEventListener('ntp-open-news-feed-settings', () => {
      if (newsEnabled) {
        setSettingsView('default')
      }
    })
  }, [newsEnabled])

  if (!showNewsFeed && !props.standalone) {
    return null
  }

  if (!newsEnabled) {
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

  return <>
    <FeedContainer standalone={props.standalone}>
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
                <FeedNav
                  onAddChannelClick={() => setSettingsView('default')}
                  onAddPublisherClick={() => setSettingsView('popular')}
                  onFeedSelect={() => setNavPopoverOpen(false)}
                />
              </div>
          }
        </div>
        <div className='feed-items'>
          <FeedItemList
            feedItems={feedItems}
            feedError={feedError}
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
                <FeedNav
                  onAddChannelClick={() => setSettingsView('default')}
                  onAddPublisherClick={() => setSettingsView('popular')}
                  onFeedSelect={() => setNavPopoverOpen(false)}
                />
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
      {props.children}
    </div>
  )
}

interface FeedItemListProps {
  feedItems: FeedItemV2[] | null
  feedError: Optional<FeedV2Error>
}

function FeedItemList(props: FeedItemListProps) {
  const { getString } = useLocale()
  const [displayCount, setDisplayCount] = React.useState(itemPageSize)

  const loadMore = React.useCallback(() => {
    setDisplayCount((count) => count + itemPageSize)
  }, [])

  const { feedItems, feedError } = props
  let articleIndex = 0

  function getArticleIndex(step: number) {
    const index = articleIndex
    articleIndex += step
    return index
  }

  if (!feedItems) {
    return (
      <div className='feed-card loading'>
        <ProgressRing />
      </div>
    )
  }

  if (feedError.hasValue()) {
    return <FeedErrorCard error={feedError.value()} />
  }

  return <>
    <ScrollReset />
    {
      feedItems.slice(0, displayCount).map((item, i) => (
        <div key={i} className='feed-card'>
          <FeedItemCard item={item} getArticleIndex={getArticleIndex} />
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

interface FeedItemCardProps {
  item: FeedItemV2
  getArticleIndex: (step: number) => number
}

function FeedItemCard(props: FeedItemCardProps) {
  const { item, getArticleIndex } = props
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

function ScrollReset() {
  const ref = React.useRef<HTMLDivElement>(null)

  // Ensure that when this component is first displayed, scroll position is not
  // set further down the page.
  React.useEffect(() => {
    const parent = ref.current?.closest('[data-css-scope]')
    if (!ref.current || !parent) {
      return
    }
    const topControlBarHeight = ref.current.offsetHeight
    const top = parent.getBoundingClientRect().y - topControlBarHeight
    if (top < 0) {
      window.scrollTo({ top: top + window.scrollY })
    }
  }, [])

  return <div ref={ref} className='scroll-reset' />
}
