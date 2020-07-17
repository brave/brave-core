// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature Containers
import CardLarge from './cards/_articles/cardArticleLarge'
import CardSmall from './cards/_articles/cardArticleMedium'
import CardBrandedList from './cards/brandedList'
import CardOrderedList from './cards/orderedList'
import CardDeals from './cards/cardDeals'

import * as BraveTodayElement from './default'

// Helpers
import { chunknizer, filterFeedFromFeaturedTypes, filterFeedByInclusion } from '../../../reducers/today/braveTodayUtils'

interface Props {
  content: BraveToday.ScrollingList
  featuredPublisher: BraveToday.OneOfPublishers
  featuredCategory: string // TODO: have a list
  parentIndex: number
}

interface State {
  setFocusOnBraveToday: boolean
}

class CardsGroup extends React.PureComponent<Props, State> {
  state = {
    setFocusOnBraveToday: false
  }
  get currentFeed () {
    // TODO: move all this to backend
    const { content, featuredPublisher, featuredCategory } = this.props
    return {
      sponsors: chunknizer(content.sponsors, 4),
      deals: chunknizer(content.deals, 3),
      media: chunknizer(content.media, 3),
      articlesRandom: chunknizer(
        filterFeedFromFeaturedTypes(
          content.articles,
          featuredPublisher,
          featuredCategory
        ),
      3),
      articlesByPublisher: chunknizer(
        filterFeedByInclusion(
          content.articles, { publisher_id: featuredPublisher }
        ),
      3),
      articlesByCategory: chunknizer(
        filterFeedByInclusion(
          content.articles, { category: featuredCategory }
        ),
      3),
      // TODO: delete
      articles: chunknizer(content.articles, 3),
    }
  }

  render () {
    const { parentIndex } = this.props
    // 1. random, 1
    // 2. random, 2
    // 3. same source, 3
    // 4. same category (dont repeat), 3
    // 5. deals, 3
    // 6. sponsored, 1
    // 7. sponsored, 3
    // 8. video cards?
    return (
      <BraveTodayElement.ArticlesGroup>
        {
          // RANDOM FEATURED ARTICLE
          this.currentFeed.articlesRandom[parentIndex] &&
          this.currentFeed.articlesRandom[parentIndex].length >= 1
            ? (
              <CardLarge
                content={[this.currentFeed.articlesRandom[parentIndex][0]]}
              />
            ) : null
        }
        {
          // RANDOM ARTICLE
          this.currentFeed.articlesRandom[parentIndex] &&
          this.currentFeed.articlesRandom[parentIndex].length >= 2
            ? (
              <CardSmall
                content={[
                  this.currentFeed.articlesRandom[parentIndex][1] || [],
                  this.currentFeed.articlesRandom[parentIndex][2] || []
                ]}
              />
            ) : null
        }
        {
          // FEATURED PUBLISHER
          this.currentFeed.articlesByPublisher[parentIndex] &&
          this.currentFeed.articlesByPublisher[parentIndex].length >= 3
            ? (
              <CardBrandedList
                content={[
                  this.currentFeed.articlesByPublisher[parentIndex][0] || [],
                  this.currentFeed.articlesByPublisher[parentIndex][1] || [],
                  this.currentFeed.articlesByPublisher[parentIndex][2] || []
                ]}
              />
            ) : null
        }
        {
          // FEATURED CATEGORY
          this.currentFeed.articlesByCategory[parentIndex] &&
          this.currentFeed.articlesByCategory[parentIndex].length >= 3
            ? (
              <CardOrderedList
                content={[
                  this.currentFeed.articlesByCategory[parentIndex][0] || [],
                  this.currentFeed.articlesByCategory[parentIndex][1] || [],
                  this.currentFeed.articlesByCategory[parentIndex][2] || []
                ]}
              />
            ) : null
        }
        {
          // DEALS
          this.currentFeed.deals[parentIndex] &&
          this.currentFeed.deals[parentIndex].length >= 3
            ? (
              <CardDeals
                content={[
                  this.currentFeed.deals[parentIndex][0] || [],
                  this.currentFeed.deals[parentIndex][1] || [],
                  this.currentFeed.deals[parentIndex][2] || []
                ]}
              />
            ) : null
        }
        {
          // FEATURED SPONSOR
          this.currentFeed.sponsors[parentIndex] &&
          this.currentFeed.sponsors[parentIndex].length >= 1
            ? (
              <CardLarge
                content={[this.currentFeed.sponsors[parentIndex][0]]}
              />
            ) : null
        }
        {
          // SPONSORS LIST
          this.currentFeed.sponsors[parentIndex] &&
          this.currentFeed.sponsors[parentIndex].length >= 4
            ? (
              <CardOrderedList
                content={[
                  this.currentFeed.sponsors[parentIndex][1] || [],
                  this.currentFeed.sponsors[parentIndex][2] || [],
                  this.currentFeed.sponsors[parentIndex][3] || []
                ]}
              />
            ) : null
        }
      </BraveTodayElement.ArticlesGroup>
    )
  }
}

export default CardsGroup
