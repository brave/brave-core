// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as BraveNews from '../../../api/brave_news'
import CardLarge from './cards/_articles/cardArticleLarge'
import CardSmall from './cards/_articles/cardArticleMedium'
import CategoryGroup from './cards/categoryGroup'
import PublisherGroup from './cards/publisherGroup'
import CardDeals from './cards/cardDeals'
import CardDisplayAd from './cards/displayAd'
import { attributeNameCardCount, GetDisplayAdContent, OnPromotedItemViewed, OnReadFeedItem, OnSetPublisherPref, OnViewedDisplayAd, OnVisitDisplayAd } from './'

import CardType = BraveNews.CardType

// Disabled rules because we have a function
// which returns elements in a switch.
// tslint:disable:jsx-wrap-multiline jsx-alignment

type Props = {
  content: BraveNews.FeedPage
  publishers: BraveNews.Publishers
  articleToScrollTo?: BraveNews.FeedItemMetadata
  shouldScrollToDisplayAd: boolean
  itemStartingDisplayIndex: number
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
  onPeriodicCardViews: (element: HTMLElement | null) => void
  onPromotedItemViewed: OnPromotedItemViewed
  onVisitDisplayAd: OnVisitDisplayAd
  onViewedDisplayAd: OnViewedDisplayAd
  getDisplayAdContent: GetDisplayAdContent
}

function getCard (props: Props, content: BraveNews.FeedPageItem) {
  switch (content.cardType) {
    case CardType.HEADLINE:
      return <CardLarge
              content={content.items}
              publishers={props.publishers}
              articleToScrollTo={props.articleToScrollTo}
              onReadFeedItem={props.onReadFeedItem}
              onSetPublisherPref={props.onSetPublisherPref}
      />
    case CardType.HEADLINE_PAIRED:
      return <CardSmall
              content={content.items}
              publishers={props.publishers}
              articleToScrollTo={props.articleToScrollTo}
              onReadFeedItem={props.onReadFeedItem}
              onSetPublisherPref={props.onSetPublisherPref}
      />
    case CardType.PROMOTED_ARTICLE:
      return <CardLarge
                isPromoted={true}
                content={content.items}
                publishers={props.publishers}
                articleToScrollTo={props.articleToScrollTo}
                onReadFeedItem={props.onReadFeedItem}
                onSetPublisherPref={props.onSetPublisherPref}
                onItemViewed={props.onPromotedItemViewed}
      />
    case CardType.CATEGORY_GROUP:
      const categoryName = content.items[0]?.article?.data.categoryName
      if (!categoryName) {
        return null
      }
      return <CategoryGroup
        content={content.items}
        publishers={props.publishers}
        categoryName={categoryName}
        onReadFeedItem={props.onReadFeedItem}
        articleToScrollTo={props.articleToScrollTo}
      />
    case CardType.PUBLISHER_GROUP:
      const publisherId = content.items[0]?.article?.data.publisherId
      if (!publisherId) {
        return null
      }
      const publisher = props.publishers[publisherId]
      return <PublisherGroup
        content={content.items}
        publisher={publisher}
        articleToScrollTo={props.articleToScrollTo}
        onReadFeedItem={props.onReadFeedItem}
        onSetPublisherPref={props.onSetPublisherPref}
      />
    case CardType.DEALS:
      return <CardDeals
        content={content.items}
        onReadFeedItem={props.onReadFeedItem}
        articleToScrollTo={props.articleToScrollTo}
      />
    case CardType.DISPLAY_AD:
      return <CardDisplayAd
        shouldScrollIntoView={props.shouldScrollToDisplayAd}
        onViewedDisplayAd={props.onViewedDisplayAd}
        onVisitDisplayAd={props.onVisitDisplayAd}
        getContent={props.getDisplayAdContent}
      />
  }
  console.error('Asked to create unknown card type', content.cardType)
  return null
}

export default function CardsGroups (props: Props) {
  // Duplicate array so we can splice without affecting state
  return <>{props.content.items.map((feedPageItem, index) => {
    const cardInstance = getCard(props, feedPageItem)
    if (!cardInstance) {
      return <React.Fragment key={index} />
    }
    // All buckets are divisible by 4, so we send a ping when we are in
    // the next bucket. This is to avoid too many redux action firing
    // on scroll.
    const shouldTriggerViewCountUpdate = (index % 4 === 1)
    return (
      <React.Fragment
        key={index}
      >
        {cardInstance}
        {shouldTriggerViewCountUpdate &&
          <div {...{ [attributeNameCardCount]: index }} ref={props.onPeriodicCardViews} />
        }
      </React.Fragment>
    )
  })}</>
}
