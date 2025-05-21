/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { getString } from '../../lib/strings'
import { WidgetMenu } from './widget_menu'
import { SafeImage } from '../common/safe_image'
import { Link } from '../common/link'
import { CategoryIcon } from '../news/category_icon'
import { CategoryName } from '../news/category_name'
import { useNewsState, useNewsActions } from '../../context/news_context'
import { dispatchCustomEvent } from '../../lib/custom_event'

import {
  FeedItemMetadata,
  FeedItemV2,
  getNewsPublisherName,
  getNewsItemImage,
} from '../../state/news_state'

import { style } from './news_widget.style'

export function NewsWidget() {
  const actions = useNewsActions()
  const isOptedIn = useNewsState((s) => s.isOptedIn)

  React.useEffect(() => {
    if (isOptedIn) {
      setTimeout(() => actions.onNewsVisible(), 250)
    }
  }, [isOptedIn])

  return (
    <div data-css-scope={style.scope}>
      <WidgetMenu>
        {isOptedIn && (
          <leo-menu-item
            onClick={() => {
              dispatchCustomEvent('ntp-open-news-feed-settings')
            }}
          >
            <Icon name='tune' />
            {getString(S.NEW_TAB_NEWS_CUSTOMIZE_BUTTON_LABEL)}
          </leo-menu-item>
        )}
        <leo-menu-item onClick={() => actions.setShowOnNTP(false)}>
          <Icon name='disable-outline' />
          {getString(S.NEW_TAB_NEWS_DISABLE_BUTTON_LABEL)}
        </leo-menu-item>
      </WidgetMenu>
      <div className='title'>{getString(S.NEW_TAB_NEWS_WIDGET_TITLE)}</div>
      {isOptedIn ? <PeekItem /> : <OptIn />}
    </div>
  )
}

function PeekItem() {
  const feedItems = useNewsState((s) => s.feedItems)

  React.useEffect(() => {
    if (feedItems) {
      cacheNewsItem(getPeekItem(feedItems))
    }
  }, [feedItems])

  const peekItem = React.useMemo(() => {
    return feedItems ? getPeekItem(feedItems) : loadCachedNewsItem()
  }, [feedItems])

  if (!peekItem) {
    return (
      <div className='peek loading'>
        <div className='img skeleton' />
        <div className='text'>
          <div className='meta skeleton' />
          <div className='item-title skeleton' />
        </div>
      </div>
    )
  }

  return (
    <Link
      url={peekItem.url.url}
      className='peek'
    >
      <SafeImage
        src={getNewsItemImage(peekItem)}
        targetSize={{ width: 71, height: 48 }}
      />
      <div>
        <div className='meta'>
          <span>{getNewsPublisherName(peekItem)}</span>
          <span>•</span>
          <CategoryIcon category={peekItem.categoryName} />
          <span>
            <CategoryName category={peekItem.categoryName} />
          </span>
        </div>
        <div className='item-title'>{peekItem.title}</div>
      </div>
    </Link>
  )
}

function OptIn() {
  const actions = useNewsActions()
  return (
    <div className='opt-in'>
      <div className='graphic' />
      <div className='text'>{getString(S.BRAVE_NEWS_INTRO_TITLE)}</div>
      <div className='actions'>
        <Button
          size='small'
          onClick={() => {
            actions.setIsOptedIn(true)
          }}
        >
          {getString(S.BRAVE_NEWS_OPT_IN_ACTION_LABEL)}
        </Button>
      </div>
    </div>
  )
}

function getPeekItem(feedItems: FeedItemV2[]): FeedItemMetadata | null {
  if (!feedItems) {
    return null
  }
  for (const item of feedItems) {
    if (item.article) {
      return item.article.data
    }
    if (item.hero) {
      return item.hero.data
    }
  }
  return null
}

const cacheKey = 'ntp-news-widget-item'

function cacheNewsItem(data: FeedItemMetadata | null) {
  const value = { cachedAt: Date.now(), data }
  localStorage.setItem(
    cacheKey,
    JSON.stringify(value, (_, value) => {
      return typeof value === 'bigint' ? value.toString() : value
    }),
  )
}

function loadCachedNewsItem(): FeedItemMetadata | null {
  let entry: any = null
  try {
    entry = JSON.parse(localStorage.getItem(cacheKey) ?? '')
  } catch {}
  if (!entry) {
    return null
  }
  const cachedAt = Number(entry.cachedAt) || 0
  if (cachedAt < Date.now() - 1000 * 60 * 60) {
    return null
  }
  const { data } = entry
  return data as FeedItemMetadata | null
}
