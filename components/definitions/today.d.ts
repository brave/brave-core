// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

declare namespace BraveToday {

  // Messages
  namespace Messages {
    // getFeed
    export type GetFeedResponse = {
      feed?: Feed
    }
    // getPublishers
    export type GetPublishersResponse = {
      publishers: Publishers | undefined
    }
    // getImageData
    export type GetImageDataPayload = {
      url: string
    }
    export type GetImageDataResponse = {
      dataUrl: string
    }
    // set publisher prefs
    export type SetPublisherPrefPayload = {
      publisherId: string
      // Boolean for explicit change, null for remove pref.
      enabled: boolean | null
    }
    export type ClearPrefsPayload = {}
    export type ClearPrefsResponse = GetPublishersResponse
    // isFeedUpdateAvailable
    export type IsFeedUpdateAvailablePayload = {
      hash: string
    }
    export type IsFeedUpdateAvailableResponse = {
      isUpdateAvailable: boolean
    }
  }

  export type FeedItem = (Article | Deal | PromotedArticle)

  export interface Feed {
    hash: string
    pages: Page[]
    featuredSponsor?: Article
    featuredArticle?: Article
  }

  export interface Page {
    articles: Article[] // 15
    randomArticles: Article[] // 3
    itemsByCategory?: {
      categoryName: string
      items: Article[] // 3
    }
    itemsByPublisher?: {
      name: string,
      items: Article[] // 3
    }
    deals: Deal[] // 3
    promotedArticle?: PromotedArticle
  }

  export interface ScrollingList {
    sponsors: Article[]
    deals: Deal[]
    articles: Article[]
  }

  export type DisplayAd = {
    uuid: string
    creativeInstanceId: string
    dimensions: string
    title: string
    description: string // Advertiser name
    imageUrl: string
    targetUrl: string
    ctaText?: string
  }

  type BaseFeedItem = {
    content_type: 'article' | 'product' | 'brave_partner'
    category: string // 'Tech', 'Business', 'Top News', 'Crypto', 'Cars', 'Culture', 'Fashion', 'Sports', 'Entertainment'
    publish_time: string // UTC "2020-04-17 19:21:10"
    title: string // "14 Truly Incredible Catfish Makeup Transformations From TikTok"
    description: string // "# Makeup skill level: Expert.↵↵![](https://img.buzzfeed.com/buzzfeed-↵static/static/2020-04/6/20/enhanced/a3cd932e6db6/original-567-1586204318-9.jpg?crop=1244:829;0,0)↵↵* * *↵↵[View Entire Post ›](https://www.buzzfeed.com/kristatorres/13-truly-↵incredible-catfish-makeup-transformations)↵↵"
    url: string // "https://www.buzzfeed.com/kristatorres/13-truly-incredible-catfish-makeup-transformations"
    url_hash: string // '0e57ac...'
    padded_img: string
    img: string // '',
    publisher_id: string // 'afd9...'
    publisher_name: string
    score: number
    relative_time?: string
    // Custom for this application. Does not come from source
    points?: number
  }

  export type Article = BaseFeedItem & {
    content_type: 'article'
  }

  export type PromotedArticle = BaseFeedItem & {
    content_type: 'brave_partner',
    creative_instance_id: string // (Possibly but not guaranteed unique) Id for this specific item
  }

  export type Deal = BaseFeedItem & {
    content_type: 'product'
    offers_category: string // 'Companion Products
  }

  export type Publisher = {
    publisher_id: string,
    publisher_name: string,
    category: string,
    enabled: boolean
    user_enabled: boolean | null
  }

  export type Publishers = {
    // tslint:disable-next-line
    [publisher_id: string]: Publisher
  }
}
