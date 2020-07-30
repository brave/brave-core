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
): Promise<BraveToday.Feed | undefined> {
  // const BraveTodayWhitelist = getBraveTodayWhitelist(topSites)
  const sponsors: (BraveToday.Article)[] = []
  const deals: (BraveToday.Deal)[] = []
  let articles: (BraveToday.Article)[] = []

  for (const feedItem of feedContent) {
    switch (feedItem.content_type) {
      case 'offer':
        sponsors.push(generateFeedProperties(feedItem) as BraveToday.Article)
        break
      case 'product':
        deals.push(generateFeedProperties(feedItem) as BraveToday.Deal)
        break
      case 'article':
        articles.push(generateFeedProperties(feedItem) as BraveToday.Article)
        break
    }
  }

  articles = await weightArticles(articles)

  // Get unique categories present
  // const categoryCounts = new Map<string, number>()
  // for (const article of articles) {
  //   if (article.category) {
  //     const existingCount = categoryCounts.get(article.category) || 0
  //     categoryCounts.set(article.category, existingCount + 1)
  //   }
  // }
  // Ordered by # of occurrences
  // const categoriesByPriority = Array.from(categoryCounts.keys()).sort((a, b) =>
  //   categoryCounts[a] - categoryCounts[b])

  // .sponsor,
  // .headline(paired: false),
  // .deals,
  const firstSponsors = sponsors.splice(0, 1) // Featured sponsor is the first sponsor
  const firstHeadlines = sponsors.splice(0, 1) // Featured article is the first sponsor
  const firstDeals = deals.splice(0, 3)

  // generate as many pages of content as possible
  const pages: BraveToday.Page[] = []
  let canGenerateAnotherPage = true
  // Sanity check: arbitrary max pages so we don't end up
  // in infinite loop.
  const maxPages = 4000;
  let curPage = 0;
  while (canGenerateAnotherPage) {
    curPage++
    if (curPage > maxPages) break
    const nextPage = generateNextPage(articles, deals)
    if (!nextPage) {
      canGenerateAnotherPage = false
      continue
    }
    pages.push(nextPage)
  }

  return {
    featuredSponsor: firstSponsors.length ? firstSponsors[0] : undefined,
    featuredArticle: firstHeadlines.length ? firstHeadlines[0] : undefined,
    featuredDeals: firstDeals,
    pages
  }
}

function getArticleHasImage (article: BraveToday.Article) {
  return !!article.img
}

function generateNextPage (articles: BraveToday.Article[], allDeals: BraveToday.Deal[]): BraveToday.Page | null {
  // .repeating([.headline(paired: false)], times: 2),
  // .repeating([.headline(paired: true)], times: 2),
  // .categoryGroup,
  // .headline(paired: false),
  // .deals,
  // .headline(paired: false),
  // .headline(paired: true),
  // .brandedGroup(numbered: true),
  // .group,
  // .headline(paired: false),
  // .headline(paired: true),

  // Collect headlines
  const headlines = take(articles, getArticleHasImage, 16)
  if (!headlines) {
    return null
  }
  const categoryInfo = generateArticleCategoryGroup(articles)
  const deals = allDeals.splice(0, 3)
  const publisherInfo = generateArticleSourceGroup(articles)
  return {
    headlines,
    deals,
    itemsByCategory: categoryInfo ? { categoryName: categoryInfo[0], items: categoryInfo[1] } : undefined,
    itemsByPublisher: publisherInfo ? { name: publisherInfo[0], items: publisherInfo[1] } : undefined,
  }
}

function generateArticleCategoryGroup (articles: BraveToday.Article[]): [string, BraveToday.Article[]] | undefined {
  const firstArticleWithCategory = articles.find(a => !!a.category)
  if (firstArticleWithCategory) {
    return [
      firstArticleWithCategory.category,
      take(
        articles,
        a => a.category === firstArticleWithCategory.category,
        3
      )
    ]
  }
  return undefined
}

function generateArticleSourceGroup (articles: BraveToday.Article[]): [string, BraveToday.Article[]] | undefined {
  const firstArticleWithSource = articles.find(a => !!a.publisher_id)
  if (firstArticleWithSource) {
    return [
      firstArticleWithSource.publisher_id,
      take(
        articles,
        a => a.publisher_id === firstArticleWithSource.publisher_id,
        3
      )
    ]
  }
  return undefined
}

function take<T>(items: T[], matching: (item: T) => boolean, count?: number): T[] {
  let indicesToTake: number[] = []
  for (const [i, item] of items.entries())  {
    const shouldTake = matching(item)
    if (!shouldTake) {
      continue
    }
    // Take item
    indicesToTake.push(i)
    // Stop, if we're limiting the count
    if (count && indicesToTake.length === count) {
      break
    }
  }
  // Get items and remove from array
  indicesToTake = indicesToTake.reverse()
  const takenItems: T[] = []
  for (const i of indicesToTake) {
    takenItems.push(...items.splice(i, 1))
  }
  return takenItems
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
