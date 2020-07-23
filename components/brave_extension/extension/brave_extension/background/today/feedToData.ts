// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { formatDistance, formatDistanceToNow } from 'date-fns'

export function generateRelativeTimeFormat (publishTime: string) {
  if (!publishTime) {
    return
  }
  return formatDistanceToNow(
    new Date(publishTime)
  ) + ' ago'
}

function cleanFeedItem(item: BraveToday.IContentItem): BraveToday.IContentItem {
  return {
    ...item,
    // img: '',
    seen: false,
    publish_time: formatDistance(
      new Date(item.publish_time),
      new Date(Date.now())
    ),
    relative_time: generateRelativeTimeFormat(item.publish_time)
  }
}

function generateFeedProperties (
  content: BraveToday.IContentItem,
  points?: number
): BraveToday.IContentItem {
  content = cleanFeedItem(content)
  if ('partner_id' in content) {
    return content
  }
  return {
    ...content,
    points: points || 0
  } as BraveToday.Article
}

export default function getBraveTodayData (
  feedContent: BraveToday.ContentFromFeed[]
): BraveToday.Feed {
  // const BraveTodayWhitelist = getBraveTodayWhitelist(topSites)
  const sponsors: (BraveToday.Article)[] = []
  const deals: (BraveToday.Deal)[] = []
  const media: (BraveToday.Media)[] = []
  const articles: (BraveToday.Article)[] = []

  for (const feedItem of feedContent) {
    if ('partner_id' in feedItem) {
      deals.push(feedItem)
    } else {
      if (feedItem.content_type === 'product') {
        sponsors.push(generateFeedProperties(feedItem) as BraveToday.Article)
      }
      if (feedItem.content_type === 'article') {
        articles.push(generateFeedProperties(feedItem) as BraveToday.Article)
      }
      if (feedItem.content_type === 'image') {
        media.push(generateFeedProperties(feedItem) as BraveToday.Article)
      }
    }
  }

  const featuredDeals = []
  if (deals.length > 0) {
    featuredDeals.push(deals[0])
    if (deals.length > 1) {
      featuredDeals.push(deals[1])
    }
  }

  return {
    featuredSponsor: sponsors[0], // Featured sponsor is the first sponsor
    featuredArticle: articles[0], // Featured article is the first sponsor
    featuredDeals,
    scrollingList: {
      sponsors: sponsors.slice(0), // Remove the first item which is used as a featured sponsor
      deals: deals.slice(1), // Remove the first two items which are used as featured deals
      media, // TBD
      articles: articles.slice(1) // Remove the first item which is used as a featured article
    }
  }
}
