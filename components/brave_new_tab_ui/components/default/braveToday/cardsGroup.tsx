// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature Containers
import CardLarge from './cards/_articles/cardArticleLarge'
import CardSmall from './cards/_articles/cardArticleMedium'
import CategoryGroup from './cards/categoryGroup'
import PublisherGroup from './cards/publisherGroup'
import CardDeals from './cards/cardDeals'
import CardDisplayAd from './cards/displayAd'
import { attributeNameCardCount, GetDisplayAdContent, OnPromotedItemViewed, OnReadFeedItem, OnSetPublisherPref, OnViewedDisplayAd, OnVisitDisplayAd } from './'

// Disabled rules because we have a function
// which returns elements in a switch.
// tslint:disable:jsx-wrap-multiline jsx-alignment

enum CardType {
  Headline,
  HeadlinePaired,
  CategoryGroup,
  PublisherGroup,
  Deals,
  DisplayAd,
  PromotedArticle
}

const PageContentOrder = [
  CardType.Headline,
  CardType.Headline,
  CardType.HeadlinePaired,
  CardType.PromotedArticle,
  CardType.CategoryGroup,
  CardType.Headline,
  CardType.Headline,
  CardType.HeadlinePaired,
  CardType.HeadlinePaired,
  CardType.DisplayAd,
  CardType.Headline,
  CardType.Headline,
  CardType.PublisherGroup,
  CardType.HeadlinePaired,
  CardType.Headline,
  CardType.Deals
] // Requires 15 headlines

const RandomContentOrder = [
  CardType.Headline,
  CardType.HeadlinePaired
] // Requires 3 headlines

export const groupItemCount = PageContentOrder.length + RandomContentOrder.length

type Props = {
  content: BraveToday.Page
  publishers: BraveToday.Publishers
  displayAds?: BraveToday.DisplayAd[]
  articleToScrollTo?: BraveToday.FeedItem
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

function getCard (props: Props, cardType: CardType, headlines: BraveToday.Article[]) {
  switch (cardType) {
    case CardType.Headline:
      // TODO: article card should handle any number of articles and
      // adapt accordingly.
      return <CardLarge
              content={headlines.splice(0, 1)}
              publishers={props.publishers}
              articleToScrollTo={props.articleToScrollTo}
              onReadFeedItem={props.onReadFeedItem}
              onSetPublisherPref={props.onSetPublisherPref}
      />
    case CardType.HeadlinePaired:
      // TODO: handle content length < 2
      return <CardSmall
              content={headlines.splice(0, 2)}
              publishers={props.publishers}
              articleToScrollTo={props.articleToScrollTo}
              onReadFeedItem={props.onReadFeedItem}
              onSetPublisherPref={props.onSetPublisherPref}
      />
    case CardType.PromotedArticle:
      if (!props.content.promotedArticle) {
        return null
      }
      return <CardLarge
                isPromoted={true}
                content={[props.content.promotedArticle]}
                publishers={props.publishers}
                articleToScrollTo={props.articleToScrollTo}
                onReadFeedItem={props.onReadFeedItem}
                onSetPublisherPref={props.onSetPublisherPref}
                onItemViewed={props.onPromotedItemViewed}
      />
    case CardType.CategoryGroup:
      if (!props.content.itemsByCategory) {
        return null
      }
      const categoryName = props.content.itemsByCategory.categoryName
      return <CategoryGroup
        content={props.content.itemsByCategory.items}
        publishers={props.publishers}
        categoryName={categoryName}
        onReadFeedItem={props.onReadFeedItem}
        articleToScrollTo={props.articleToScrollTo}
      />
    case CardType.PublisherGroup:
      if (!props.content.itemsByPublisher) {
        return null
      }
      const publisherId = props.content.itemsByPublisher.name
      const publisher = props.publishers[publisherId]
      return <PublisherGroup
        content={props.content.itemsByPublisher.items}
        publisher={publisher}
        articleToScrollTo={props.articleToScrollTo}
        onReadFeedItem={props.onReadFeedItem}
        onSetPublisherPref={props.onSetPublisherPref}
      />
    case CardType.Deals:
      return <CardDeals
        content={props.content.deals}
        onReadFeedItem={props.onReadFeedItem}
        articleToScrollTo={props.articleToScrollTo}
      />
    case CardType.DisplayAd:
      return <CardDisplayAd
        shouldScrollIntoView={props.shouldScrollToDisplayAd}
        onViewedDisplayAd={props.onViewedDisplayAd}
        onVisitDisplayAd={props.onVisitDisplayAd}
        getContent={props.getDisplayAdContent}
      />
  }
  console.error('Asked to create unknown card type', cardType)
  return null
}

export default function CardsGroups (props: Props) {
  // Duplicate array so we can splice without affecting state
  const headlines = [...props.content.articles]
  const randomHeadlines = [...props.content.randomArticles]
  const groups = [
    { order: PageContentOrder, items: headlines },
    { order: RandomContentOrder, items: randomHeadlines }
  ]
  let displayIndex = props.itemStartingDisplayIndex
  return <>{groups.flatMap((group, groupIndex) => group.order.map(
    (cardType, orderIndex) => {
      const cardInstance = getCard(props, cardType, group.items)
      if (!cardInstance) {
        return null
      }
      displayIndex++
      // All buckets are divisible by 4, so we send a ping when we are in
      // the next bucket. This is to avoid too many redux action firing
      // on scroll.
      const shouldTriggerViewCountUpdate = (displayIndex % 4 === 1)
      return (
        <React.Fragment
          key={displayIndex}
        >
          {cardInstance}
          {shouldTriggerViewCountUpdate &&
            <div {...{ [attributeNameCardCount]: displayIndex }} ref={props.onPeriodicCardViews} />
          }
        </React.Fragment>
      )
    }
  ))}</>
}
