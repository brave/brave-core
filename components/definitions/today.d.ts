// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

declare namespace BraveToday {

  // Messages
  namespace Messages {
    export type GetFeedResponse = {
      feed: BraveToday.Feed | undefined
    }
  }

  export type ContentFromFeed = (Article | Deal | Media)

  export interface Feed {
    featuredSponsor: Article
    featuredArticle: Article
    featuredDeals: Deal[]
    scrollingList: ScrollingList
  }

  export interface ScrollingList {
    sponsors: Article[]
    deals: Deal[]
    media: Media[]
    articles: Article[]
  }

  interface IContentItem {
    publish_time: string // UTC "2020-04-17 19:21:10"
    title: string // "14 Truly Incredible Catfish Makeup Transformations From TikTok"
    description: string // "# Makeup skill level: Expert.↵↵![](https://img.buzzfeed.com/buzzfeed-↵static/static/2020-04/6/20/enhanced/a3cd932e6db6/original-567-1586204318-9.jpg?crop=1244:829;0,0)↵↵* * *↵↵[View Entire Post ›](https://www.buzzfeed.com/kristatorres/13-truly-↵incredible-catfish-makeup-transformations)↵↵"
    url: string // "https://www.buzzfeed.com/kristatorres/13-truly-incredible-catfish-makeup-transformations"
    img: string // '',
    // Custom for this application. Does not come from source
    relative_time?: string // "1 hour ago"
    seen?: boolean // whether or not thus content have been loaded before
  }

  export type Article = IContentItem & {
    category: string // 'Tech', 'Business', 'Top News', 'Crypto', 'Cars', 'Culture', 'Fashion', 'Sports', 'Entertainment'
    content_type: 'article' | 'product' | 'image'
    default?: boolean // true
    domain: null // ???
    publish_time: string // UTC "2020-04-17 19:21:10"
    relative_time?: string // "1 hour ago"
    publisher_id: string // "buzzfeed"
    publisher_logo: string // "https://nlbtest.rapidpacket.com/logos/buzzfeed.com.png"
    publisher_name: string // "BuzzFeed"
    // Custom for this application. Does not come from source
    points?: number
  }

  export type Media = Article

  export type Deal = IContentItem & {
    status: string // 'live',
    partner_name: string // 'LG',
    partner_id: string // '',
    partner_logo: string // '',
    category: string // 'Electronics',
    content_type: string // 'product',
    price: string // '$1,899',
    date_live_from: string // '2020-04-15 00:00:00',
    date_live_to: string // '2020-04-29 00:00:00',
    // Custom for this application. Does not come from source
    seen?: boolean // whether or not thus content have been loaded before
  }

  export type OneOfPublishers = string
    // 'buzzfeed' |
    // 'netflix' |
    // 'amazon' |
    // 'tigerdirect' |
    // 'slickdeals' |
    // 'digitaltrends' |
    // 'wirecutter' |
    // 'askreddit' |
    // 'lifehacker' |
    // 'wikihow' |
    // 'buzzfeed_quizzes' |
    // 'mentalfloss' |
    // 'wotd' |
    // 'digg' |
    // 'ign' |
    // 'nature' |
    // 'cnet' |
    // 'yahoo_sports' |
    // 'yahoo_finance' |
    // 'popsci' |
    // 'mashable' |
    // 'techcrunch' |
    // 'wired' |
    // 'nasa_photo' |
    // 'reddit' |
    // 'giphy' |
    // 'newegg' |
    // 'bbc' |
    // 'cnn' |
    // 'nytimes' |
    // 'theguardian' |
    // 'dailymail' |
    // 'washingtonpost' |
    // 'businessinsider' |
    // 'foxnews' |
    // 'reuters' |
    // 'wsj' |
    // 'npr' |
    // 'nbcnews' |
    // 'ft'

    export type OneOfCategories =
      'Fashion' | string
}
