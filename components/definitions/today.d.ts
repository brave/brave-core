// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

declare namespace BraveToday {
  export type ContentFromFeed = (Article | Deal | Media)

  export interface Feed {
    featuredSponsor: Article
    featuredArticle: Article
    featuredDeals: Deal[]
    scrollingList: ScrollingList
  }

  export interface State {
    feed: Feed
  }

  export interface ApplicationState {
    braveTodayData: State | undefined
  }

  export interface ScrollingList {
    sponsors: Article[]
    deals: Deal[]
    media: Media[]
    articles: Article[]
  }

  export type Article = {
    category: string // 'Tech', 'Business', 'Top News', 'Crypto', 'Cars', 'Culture', 'Fashion', 'Sports', 'Entertainment'
    content_type: 'article' | 'product' | 'image'
    default?: boolean // true
    description: string // "# Makeup skill level: Expert.↵↵![](https://img.buzzfeed.com/buzzfeed-↵static/static/2020-04/6/20/enhanced/a3cd932e6db6/original-567-1586204318-9.jpg?crop=1244:829;0,0)↵↵* * *↵↵[View Entire Post ›](https://www.buzzfeed.com/kristatorres/13-truly-↵incredible-catfish-makeup-transformations)↵↵"
    domain: null // ???
    img: string // "https://nlbtest.rapidpacket.com/cache/c1d3693ae39f4cb8fc4d36ca25445cccf89d6561"
    publish_time: string // UTC "2020-04-17 19:21:10"
    publisher_id: string // "buzzfeed"
    publisher_logo: string // "https://nlbtest.rapidpacket.com/logos/buzzfeed.com.png"
    publisher_name: string // "BuzzFeed"
    title: string // "14 Truly Incredible Catfish Makeup Transformations From TikTok"
    url: string // "https://www.buzzfeed.com/kristatorres/13-truly-incredible-catfish-makeup-transformations"
    // Custom for this application. Does not come from source
    seen?: boolean // whether or not thus content have been loaded before
    points?: number
  }

  export type Media = Article

  export type Deal = {
    status: string // 'live',
    partner_name: string // 'LG',
    partner_id: string // '',
    partner_logo: string // '',
    category: string // 'Electronics',
    content_type: string // 'product',
    title: string // '65\' Class HDR 4K UHD Smart OLED TV',
    description: string // '',
    price: string // '$1,899',
    img: string // '',
    url: string // 'https://www.newegg.com/lg-oled-c9-65/p/16C-000P-00364?Item=9SIAJKJ95U7775',
    date_live_from: string // '2020-04-15 00:00:00',
    date_live_to: string // '2020-04-29 00:00:00',
    publish_time: string // '2020-04-21 19:04:09'
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
