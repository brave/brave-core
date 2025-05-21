/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'
import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import { getString } from '../../lib/strings'
import { SafeImage } from '../common/safe_image'
import { Link } from '../common/link'
import { CategoryIcon } from '../news/category_icon'
import { CategoryName } from '../news/category_name'

import { style } from './news_widget.style'

export function NewsWidget() {
  const braveNews = useBraveNews()
  const isOptedIn = braveNews.isOptInPrefEnabled

  return (
    <div data-css-scope={style.scope}>
      <div className='title'>
        {getString('newsWidgetTitle')}
      </div>
      {isOptedIn ? <PeekItem /> : <OptIn />}
    </div>
  )
}

function PeekItem() {
  const braveNews = useBraveNews()
  const feedItems = braveNews.feedV2?.items

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
    <Link url={peekItem.url.url} className='peek'>
      <SafeImage src={getNewsItemImage(peekItem)} />
      <div>
        <div className='meta'>
          <span>{getNewsPublisherName(peekItem)}</span>
          <span>•</span>
          <CategoryIcon category={peekItem.categoryName} />
          <span><CategoryName category={peekItem.categoryName} /></span>
        </div>
        <div className='item-title'>
          {formatTitle(peekItem.title)}
        </div>
      </div>
    </Link>
  )
}

function OptIn() {
  const braveNews = useBraveNews()
  return (
    <div className='opt-in'>
      <div className='graphic' />
      <div className='text'>
        {getString('newsEnableText')}
      </div>
      <div className='actions'>
        <Button
          size='small'
          onClick={() => { braveNews.toggleBraveNewsOnNTP(true) }}
        >
          {getString('newsEnableButtonLabel')}
        </Button>
      </div>
    </div>
  )
}

function getPeekItem(
  feedItems: mojom.FeedItemV2[]
): mojom.FeedItemMetadata | null {
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

function formatTitle(title: string) {
  const maxLength = 99
  if (title.length < maxLength + 1) {
    return title
  }
  return <>{title.slice(0, maxLength)}&hellip;</>
}

const cacheKey = 'ntp-news-widget-item'

function cacheNewsItem(data: mojom.FeedItemMetadata | null) {
  const value = { cachedAt: Date.now(), data }
  localStorage.setItem(cacheKey, JSON.stringify(value, (_, value) => {
    return typeof value === 'bigint' ? value.toString() : value
  }))
}

function loadCachedNewsItem(): mojom.FeedItemMetadata | null {
  let entry: any = null
  try { entry = JSON.parse(localStorage.getItem(cacheKey) ?? '') } catch {}
  if (!entry) {
    return null
  }
  const cachedAt = Number(entry.cachedAt) || 0
  if (cachedAt < Date.now() - (1000 * 60 * 60)) {
    return null
  }
  const { data } = entry
  return data as mojom.FeedItemMetadata | null
}

function getNewsPublisherName(item: mojom.FeedItemMetadata) {
  if (item.publisherName) {
    return item.publisherName
  }
  try {
    return new URL(item.url.url).hostname
  } catch {
    return ''
  }
}

function getNewsItemImage(data: mojom.FeedItemMetadata) {
  return data.image.imageUrl?.url ?? data.image.paddedImageUrl?.url ?? ''
}
