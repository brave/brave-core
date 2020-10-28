// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import useScrollIntoView from '../../useScrollIntoView'
import CardImage from '../CardImage'
import * as Card from './style'

interface Props {
  content: (BraveToday.Article)[]
  publishers: BraveToday.Publishers
  categoryName: string
  articleToScrollTo?: BraveToday.FeedItem
  onReadFeedItem: (item: BraveToday.FeedItem) => any
}

type ListItemProps = {
  item: BraveToday.Article
  publisher?: BraveToday.Publisher
  onReadFeedItem: (item: BraveToday.FeedItem) => any
  shouldScrollIntoView: boolean
}

function ListItem (props: ListItemProps) {
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView)
  const onClick = React.useCallback(() => {
    props.onReadFeedItem(props.item)
  }, [props.onReadFeedItem, props.item])
  return (
    <Card.ListItem>
      <a onClick={onClick} href={props.item.url} ref={cardRef}>
        <Card.Content>
          <Card.Heading>{props.publisher && props.publisher.publisher_name}</Card.Heading>
          <Card.Heading>{props.item.title}</Card.Heading>
          <Card.Time>{props.item.relative_time}</Card.Time>
        </Card.Content>
        <Card.ListItemImageFrame>
          <CardImage list={true} imageUrl={props.item.img} />
        </Card.ListItemImageFrame>
      </a>
    </Card.ListItem>
  )
}

export default function CategoryGroup (props: Props) {
  // No content no render®
  if (props.content.length < 3) {
    return null
  }
  return (
    <Card.BrandedList>
      <Card.Title>{props.categoryName}</Card.Title>
      <Card.List>
        {
          props.content.map((item, index) => {
            const shouldScrollTo = (
              !!props.articleToScrollTo &&
              props.articleToScrollTo.url === item.url
            )
            const publisher = props.publishers[item.publisher_id]
            return <ListItem
              publisher={publisher}
              item={item}
              key={index}
              shouldScrollIntoView={shouldScrollTo}
              onReadFeedItem={props.onReadFeedItem}
            />
          })
        }
      </Card.List>
    </Card.BrandedList>
  )
}
