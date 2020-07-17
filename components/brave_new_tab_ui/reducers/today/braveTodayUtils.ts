import { formatDistanceToNow } from 'date-fns'

export function chunknizer (arr: any[], chunkSize: number) {
  return arr.reduce((acc, _, i) => {
    if (i % chunkSize === 0) {
      acc.push(arr.slice(i, i + chunkSize))
    }
    return acc
  }, [])
}

export function generateFeedProperties (
  content: BraveToday.Article,
  points?: number
): BraveToday.Article {
  if ('partner_id' in content) {
    return content
  }
  return {
    ...content,
    seen: false,
    points: points || 0
  }
}

// Given a type, returns all elements that include it
export function filterFeedByInclusion (
  feed: BraveToday.Article[],
  type: { publisher_id: BraveToday.OneOfPublishers } | { category: string } | { content_type: 'product' | 'image' | 'article' }
) {
  const typeKey = Object.keys(type)[0]
  const typeValue = Object.values(type)[0]
  return feed.filter(item => item && item[typeKey] === typeValue)
}

// Get the all the feed excluding types features by default in each list.
// This prevents us from having duplicated entries being shown
// as default items and as featured items (inside a list).
export function filterFeedFromFeaturedTypes (
  feed: BraveToday.Article[],
  featuredPublisher: BraveToday.OneOfPublishers,
  featuredCategory: BraveToday.OneOfCategories
) {
  return feed.filter(item => {
    return (
      item.publisher_id !== featuredPublisher &&
      item.category !== featuredCategory
    )
  })
}

export function shuffleCategorynames () {
  // TODO return storted out category names in a array
}

export function generateRelativeTimeFormat (publishTime: string) {
  if (!publishTime) {
    return
  }
  return formatDistanceToNow(
    new Date(publishTime)
  ) + ' ago'
}
