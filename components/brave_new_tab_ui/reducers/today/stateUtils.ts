// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { generateFeedProperties } from '../helpers/braveTodayUtils'

export function braveTodayReducerSetFirstRenderData (
  state: BraveToday.State,
  feedContent: BraveToday.ContentFromFeed[]
): BraveToday.State {
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
        sponsors.push(generateFeedProperties(feedItem))
      }
      if (feedItem.content_type === 'article') {
        articles.push(generateFeedProperties(feedItem))
      }
      if (feedItem.content_type === 'image') {
        media.push(generateFeedProperties(feedItem))
      }
    }
  }

  return {
    ...state,
    feed: {
      featuredSponsor: sponsors[0], // Featured sponsor is the first sponsor
      featuredArticle: articles[0], // Featured article is the first sponsor
      featuredDeals: [ deals[0], deals[1] ], // Featured deals are the first two deals
      scrollingList: {
        sponsors: sponsors.slice(0), // Remove the first item which is used as a featured sponsor
        deals: deals.slice(1), // Remove the first two items which are used as featured deals
        media, // TBD
        articles: articles.slice(1) // Remove the first item which is used as a featured article
      }
    }
  }
}

export function braveTodayReducerDataUpdated (
  state: BraveToday.State,
  typeToUpdate: BraveToday.ContentFromFeed,
  feedContent: BraveToday.ContentFromFeed[]
): BraveToday.State {
  let updatedBraveToday: BraveToday.ContentFromFeed[] = []

  for (const feedItem of feedContent) {
    if (feedItem.seen === false) {
      updatedBraveToday.push(feedItem)
    }
  }

  state = {
    ...state,
    feed: {
      ...state.feed,
      [typeToUpdate.toString()]: updatedBraveToday
    }
  }
  return state
}
