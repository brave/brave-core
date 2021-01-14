// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../cardSizes'
import CardImage from './CardImage'
import useScrollIntoView from '../useScrollIntoView'
import useReadArticleClickHandler from '../useReadArticleClickHandler'
import { OnReadFeedItem } from '../'

interface Props {
  content: BraveToday.Deal[]
  articleToScrollTo?: BraveToday.FeedItem
  onReadFeedItem: OnReadFeedItem
}

type ListItemProps = {
  item: BraveToday.Deal
  onReadFeedItem: OnReadFeedItem
  shouldScrollIntoView: boolean
}

function ListItem (props: ListItemProps) {
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView)
  const onClick = useReadArticleClickHandler(props.onReadFeedItem, {
    item: props.item
  })
  return (
    <Card.DealItem innerRef={cardRef} onClick={onClick} href={props.item.url}>
      <CardImage imageUrl={props.item.padded_img} />
      <Card.Text>{props.item.title}</Card.Text>
      <Card.DealDescription>{props.item.description}</Card.DealDescription>
    </Card.DealItem>
  )
}

export default function CardDeals (props: Props) {
  // no full content no render®
  if (props.content.length === 0) {
    return null
  }

  return (
    <Card.DealsCard>
      <Card.DealsCategory>{props.content[0].offers_category}</Card.DealsCategory>
      <Card.DealsList>
        {
          props.content.map((item, index) => {
            const shouldScrollIntoView = (
              !!props.articleToScrollTo &&
              props.articleToScrollTo.url === item.url
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
