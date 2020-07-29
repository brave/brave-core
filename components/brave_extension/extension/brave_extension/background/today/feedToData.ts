// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { formatDistanceToNow } from 'date-fns'


function shuffleArray(array: Array<any>) {
  for (let i = array.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [array[i], array[j]] = [array[j], array[i]];
  }
}

export function generateRelativeTimeFormat (publishTime: string) {
  if (!publishTime) {
    return
  }
  // TODO(petemill): proper internationalisation syntax
  return formatDistanceToNow(
    new Date(publishTime)
  ) + ' ago'
}

function convertFeedItem(item: BraveToday.IContentItem): BraveToday.IContentItem {
  const publishTime = item.publish_time + ' UTC'
  return {
    ...item,
    publish_time: publishTime,
    relative_time: generateRelativeTimeFormat(publishTime),
    seen: false
  }
}

function generateFeedProperties (
  content: BraveToday.IContentItem,
  points?: number
): BraveToday.IContentItem {
  content = convertFeedItem(content)
  if ('partner_id' in content) {
    return content
  }
  return {
    ...content,
    points: points || 0
  } as BraveToday.Article
}

export default async function getBraveTodayData (
  feedContent: BraveToday.ContentFromFeed[]
): Promise<BraveToday.Feed> {
  // const BraveTodayWhitelist = getBraveTodayWhitelist(topSites)
  const sponsors: (BraveToday.Article)[] = []
  const deals: (BraveToday.Deal)[] = []
  const media: (BraveToday.Media)[] = []
  let articles: (BraveToday.Article)[] = []

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

  articles = await weightArticles(articles)

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

function domainFromUrlString(urlString: string): string {
  return new window.URL(urlString).host
}

async function weightArticles(articles: BraveToday.Article[]): Promise<BraveToday.Article[]> {
  // hosts from latest max-200 history items
  const historyItems: chrome.history.HistoryItem[] = await new Promise(resolve => chrome.history.search({ text: '', maxResults: 200 }, resolve))
  const historyHosts: string[] = historyItems.map(h => h.url).filter(i => i).map((u: string) => domainFromUrlString(u))
  for (const article of articles) {
    const secondsSincePublish = Math.abs((new Date().getTime() - new Date(article.publish_time).getTime()) / 1000)
    console.log('secondsSince Publish', article.publish_time, secondsSincePublish)
    let score = Math.log(secondsSincePublish)
    const hasDomainHistoryMatch = historyHosts.includes(domainFromUrlString(article.url))
    if (hasDomainHistoryMatch) {
      score -= 5
    }
    article.points = score
  }
  articles = articles.sort((a, b) => (a.points || 1000) - (b.points || 1000))

  const topArticles = articles.splice(0, 25)
  shuffleArray(topArticles)
  return [
    ...topArticles,
    ...articles
  ]
}
