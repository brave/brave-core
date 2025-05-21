/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import * as news from './news_state'

describe('getNewsPublisherName', () => {
  const url = 'https://brave.com/a/b/c'
  it('should return publisherName if available', () => {
    const metadata = news.feedItemMetadata({
      publisherName: '$name',
      url: { url },
    })
    expect(news.getNewsPublisherName(metadata)).toBe('$name')
  })

  it('should return the URL hostname if available', () => {
    const metadata = news.feedItemMetadata({ url: { url } })
    expect(news.getNewsPublisherName(metadata)).toBe('brave.com')
  })

  it('should return the empty string if there is no name or url', () => {
    expect(news.getNewsPublisherName(news.feedItemMetadata({}))).toBe('')
  })
})

describe('getNewsItemImage', () => {
  const url = '$image-url'
  it('should return imageUrl if available', () => {
    const metadata = news.feedItemMetadata({
      image: {
        imageUrl: { url },
        paddedImageUrl: undefined,
      },
    })
    expect(news.getNewsItemImage(metadata)).toBe('$image-url')
  })

  it('should return paddedImageUrl if available', () => {
    const metadata = news.feedItemMetadata({
      image: {
        imageUrl: undefined,
        paddedImageUrl: { url },
      },
    })
    expect(news.getNewsItemImage(metadata)).toBe('$image-url')
  })

  it('should return the empty string if neither are present', () => {
    expect(news.getNewsItemImage(news.feedItemMetadata({}))).toBe('')
  })
})

describe('isNewsChannelEnabled', () => {
  it('should return true if there are subscribed locales', () => {
    expect(
      news.isNewsChannelEnabled({
        channelName: '',
        subscribedLocales: ['x'],
      }),
    ).toBe(true)
  })

  it('should return false if there are no subscribed locales', () => {
    expect(
      news.isNewsChannelEnabled({
        channelName: '',
        subscribedLocales: [],
      }),
    ).toBe(false)
  })
})

describe('isNewsPublisherEnabled', () => {
  const emptyPublisher = {
    publisherId: '',
    publisherName: '',
    type: mojom.PublisherType.COMBINED_SOURCE,
    categoryName: '',
    isEnabled: false,
    locales: [],
    feedSource: { url: '' },
    faviconUrl: { url: '' },
    coverUrl: { url: '' },
    backgroundColor: '',
    siteUrl: { url: '' },
    userEnabledStatus: mojom.UserEnabled.NOT_MODIFIED,
  }

  it('should return true if status is enabled', () => {
    const publisher = {
      ...emptyPublisher,
      userEnabledStatus: mojom.UserEnabled.ENABLED,
    }
    expect(news.isNewsPublisherEnabled(publisher)).toBe(true)
  })

  it('should return false if status is disabled', () => {
    const publisher = {
      ...emptyPublisher,
      userEnabledStatus: mojom.UserEnabled.DISABLED,
    }
    expect(news.isNewsPublisherEnabled(publisher)).toBe(false)
  })

  it('should return false if status is not modfied and combined', () => {
    const publisher = {
      ...emptyPublisher,
      type: mojom.PublisherType.COMBINED_SOURCE,
    }
    expect(news.isNewsPublisherEnabled(publisher)).toBe(false)
  })

  it('should return true if status is not modfied and direct', () => {
    const publisher = {
      ...emptyPublisher,
      type: mojom.PublisherType.DIRECT_SOURCE,
    }
    expect(news.isNewsPublisherEnabled(publisher)).toBe(true)
  })
})

describe('newsFeedTypesEqual', () => {
  const { feedType } = news
  function testEqual(a: news.FeedV2Type, b: news.FeedV2Type, equal: boolean) {
    expect(news.newsFeedTypesEqual(a, b)).toBe(equal)
  }

  it('should return true if same type with same value', () => {
    testEqual(feedType('all'), feedType('all'), true)
    testEqual(feedType('following'), feedType('following'), true)
    testEqual(feedType('channel', 'a'), feedType('channel', 'a'), true)
    testEqual(feedType('publisher', 'a'), feedType('publisher', 'a'), true)
  })

  it('should return false if values are different', () => {
    testEqual(feedType('channel', 'a'), feedType('channel', 'b'), false)
    testEqual(feedType('publisher', 'a'), feedType('publisher', 'b'), false)
  })

  it('should return false if types are different', () => {
    testEqual(feedType('all'), feedType('following'), false)
    testEqual(feedType('channel', 'a'), feedType('publisher', 'a'), false)
  })
})
