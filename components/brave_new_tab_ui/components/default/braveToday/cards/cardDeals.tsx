// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import { FeedItem, FeedItemMetadata } from '../../../../api/brave_news'
import * as Card from '../cardSizes'
import { CardImageFromFeedItem } from './CardImage'
import useScrollIntoView from '../useScrollIntoView'
import useReadArticleClickHandler from '../useReadArticleClickHandler'
import { OnReadFeedItem } from '../'

interface Props {
  content: FeedItem[]
  articleToScrollTo?: FeedItemMetadata
  onReadFeedItem: OnReadFeedItem
}

type ListItemProps = {
  item: FeedItem
  onReadFeedItem: OnReadFeedItem
  shouldScrollIntoView: boolean
}

function ListItem (props: ListItemProps) {
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView)
  const onClick = useReadArticleClickHandler(props.onReadFeedItem, {
    item: props.item
  })
  const data = props.item.deal?.data
  if (!data) {
    return null
  }
  return (
    <Card.DealItem ref={cardRef} onClick={onClick} href={data.url.url}>
      <CardImageFromFeedItem data={data} />
      <Card.Text>{data.title}</Card.Text>
      <Card.DealDescription>{data.description}</Card.DealDescription>
    </Card.DealItem>
  )
}

export default function CardDeals (props: Props) {
  if (!props.content || props.content.length === 0 || !props.content[0].deal) {
    return null
  }

  const categoryName = props.content[0].deal.data.categoryName

  return (
    <Card.DealsCard>
      <Card.DealsCategory>{categoryName}</Card.DealsCategory>
      <Card.DealsList>
        {
          props.content.map((item, index) => {
            const shouldScrollIntoView = (
              !!props.articleToScrollTo &&
              props.articleToScrollTo.url.url === item.deal?.data.url.url
            )
            return <ListItem
              key={index}
              item={item}
              shouldScrollIntoView={shouldScrollIntoView}
              onReadFeedItem={props.onReadFeedItem}
            />
          })
        }
      </Card.DealsList>
    </Card.DealsCard>
  )
}
