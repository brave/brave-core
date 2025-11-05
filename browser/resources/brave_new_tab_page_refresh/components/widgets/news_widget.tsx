/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'
import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import { getTranslatedChannelName } from '../../../../../components/brave_news/browser/resources/shared/channel'
import { channelIcons } from '../../../../../components/brave_news/browser/resources/shared/Icons'

import { getString } from '../../lib/strings'
import { WidgetMenu } from './widget_menu'
import { SafeImage } from '../common/safe_image'
import { Link } from '../common/link'

import { style } from './news_widget.style'

export function NewsWidget() {
  const braveNews = useBraveNews()
  return (
    <div data-css-scope={style.scope}>
      <WidgetMenu>
        {
          braveNews.isOptInPrefEnabled &&
            <leo-menu-item onClick={() => braveNews.setCustomizePage('news')}>
              <Icon name='tune' />
              {getString(S.NEW_TAB_NEWS_CUSTOMIZE_BUTTON_LABEL)}
            </leo-menu-item>
        }
        <leo-menu-item onClick={() => braveNews.toggleBraveNewsOnNTP(false)}>
          <Icon name='disable-outline' />
          {getString(S.NEW_TAB_NEWS_DISABLE_BUTTON_LABEL)}
        </leo-menu-item>
      </WidgetMenu>
      <div className='title'>
        {getString(S.NEW_TAB_NEWS_WIDGET_TITLE)}
      </div>
      {braveNews.isOptInPrefEnabled ? <PeekItem /> : <OptIn />}
    </div>
  )
}

function PeekItem() {
  const braveNews = useBraveNews()
  const feedItems = braveNews.feedV2?.items

  const peekItem = React.useMemo(() => {
    return feedItems ? getPeekItem(feedItems) : null
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
      <SafeImage
        src={getNewsItemImage(peekItem)}
        targetSize={{ width: 71, height: 48 }}
      />
      <div>
        <div className='meta'>
          <span>{getNewsPublisherName(peekItem)}</span>
          <span>•</span>
          {channelIcons[peekItem.categoryName] ?? channelIcons.default}
          <span>
            {getTranslatedChannelName(peekItem.categoryName)}
          </span>
        </div>
        <div className='item-title'>
          {peekItem.title}
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
        {getString(S.BRAVE_NEWS_INTRO_TITLE)}
      </div>
      <div className='actions'>
        <Button
          size='small'
          onClick={() => { braveNews.toggleBraveNewsOnNTP(true) }}
        >
          {getString(S.BRAVE_NEWS_OPT_IN_ACTION_LABEL)}
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
