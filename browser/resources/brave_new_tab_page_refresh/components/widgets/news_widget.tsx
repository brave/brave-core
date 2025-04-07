/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { NewsItem, NewsFeedItem, getNewsPublisherName } from '../../models/news'
import { useLocale } from '../context/locale_context'
import { useAppActions, useAppState } from '../context/app_model_context'
import { SafeImage } from '../common/safe_image'
import { Link } from '../common/link'
import { CategoryIcon } from '../news/category_icon'
import { CategoryName } from '../news/category_name'

import { style } from './news_widget.style'

export function NewsWidget() {
  const { getString } = useLocale()
  const actions = useAppActions()

  const newsEnabled = useAppState((s) => s.newsEnabled)
  const feedItems = useAppState((s) => s.newsFeedItems)

  React.useEffect(() => {
    if (feedItems) {
      cacheNewsItem(getPeekItem(feedItems))
    }
  }, [feedItems])

  const peekItem = React.useMemo(() => {
    if (feedItems) {
      return getPeekItem(feedItems)
    }
    return loadCachedNewsItem()
  }, [feedItems])

  React.useEffect(() => {
    actions.onNewsVisible()
  }, [])

  function renderSkeleton() {
    return (
      <div className='peek loading'>
        <div className='img skeleton' />
        <div>
          <div className='meta skeleton' />
          <div className='item-title skeleton' />
        </div>
      </div>
    )
  }

  function renderPeekItem() {
    if (!peekItem) {
      return renderSkeleton()
    }

    return (
      <Link url={peekItem.url} className='peek'>
        <SafeImage src={peekItem.imageUrl} />
        <div>
          <div className='meta'>
            <span>{getNewsPublisherName(peekItem)}</span>
            <span>•</span>
            <CategoryIcon category={peekItem.categoryName} />
            <span><CategoryName category={peekItem.categoryName} /></span>
          </div>
          <div className='item-title'>
            {peekItem.title}
          </div>
        </div>
      </Link>
    )
  }

  function renderOptIn() {
    return (
      <div className='opt-in'>
        <div className='graphic' />
        <div className='text'>
          {getString('newsEnableText')}
        </div>
        <div className='actions'>
          <Button
            size='small'
            onClick={() => { actions.setNewsEnabled(true) }}
          >
            {getString('newsEnableButtonLabel')}
          </Button>
        </div>
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='title'>
        {getString('newsWidgetTitle')}
      </div>
      {newsEnabled ? renderPeekItem() : renderOptIn()}
    </div>
  )
}

function getPeekItem(feedItems: NewsFeedItem[]): NewsItem | null {
  if (!feedItems) {
    return null
  }
  const item = feedItems.find((item) => {
    return item.type === 'article' || item.type === 'hero'
  })
  return item ?? null
}

const cacheKey = 'ntp-news-widget-item'

function cacheNewsItem(item: NewsItem | null) {
  localStorage.setItem(cacheKey, JSON.stringify(item))
}

function loadCachedNewsItem(): NewsItem | null {
  let entry: any = null
  try { entry = JSON.parse(localStorage.getItem(cacheKey) || '') } catch {}
  if (!entry) {
    return null
  }

  const item: NewsItem = {
    title: String(entry.title || ''),
    categoryName: String(entry.categoryName || ''),
    publisherId: String(entry.publisherId || ''),
    publisherName: String(entry.publisherName || ''),
    url: String(entry.url || ''),
    imageUrl: String(entry.imageUrl || ''),
    relativeTimeDescription: String(entry.relativeTimeDescription || '')
  }

  if (item.title && item.url && item.categoryName && item.publisherName &&
      item.imageUrl) {
    return item
  }

  return null
}
