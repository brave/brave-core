/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import { getString } from '../../lib/strings'
import { FeedItemV2, FeedV2Error } from '../../state/news_state'
import { FeedErrorCard } from './feed_error_card'
import { ArticleCard } from './article_card'
import { ClusterCard } from './cluster_card'
import { DiscoverCard } from './discover_card'
import { VisibilityTracker } from '../common/visibility_tracker'

import { style } from './feed_item_list.style'

const itemPageSize = 25

interface Props {
  feedItems: FeedItemV2[] | null
  feedError: FeedV2Error | null
}

export function FeedItemList(props: Props) {
  const [displayCount, setDisplayCount] = React.useState(itemPageSize)

  const loadMore = React.useCallback(() => {
    setDisplayCount((count) => count + itemPageSize)
  }, [])

  const { feedItems, feedError } = props

  // Article indexes are one-based.
  let articleIndex = 1

  function getArticleIndex(step: number) {
    const index = articleIndex
    articleIndex += step
    return index
  }

  if (!feedItems) {
    return (
      <div data-css-scope={style.scope}>
        <div className='feed-card loading'>
          <ProgressRing />
        </div>
      </div>
    )
  }

  if (feedError !== null) {
    return (
      <div data-css-scope={style.scope}>
        <FeedErrorCard error={feedError} />
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <ScrollReset />
      {feedItems.slice(0, displayCount).map((item, i) => (
        <div
          key={i}
          className='feed-card'
        >
          <FeedItemCard
            item={item}
            getArticleIndex={getArticleIndex}
          />
        </div>
      ))}
      {feedItems.length > displayCount ? (
        <VisibilityTracker
          rootMargin='0px 0px 1000px 0px'
          onVisible={loadMore}
        />
      ) : (
        <div className='caught-up'>
          <hr />
          <p>
            <Icon name='check-circle-outline' />
            {getString(S.BRAVE_NEWS_CAUGHT_UP)}
          </p>
          <hr />
        </div>
      )}
    </div>
  )
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
  // set further down the page than the top of the news feed.
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

  return (
    <div
      ref={ref}
      className='scroll-reset'
    />
  )
}
