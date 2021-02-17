// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
// Feature-specific components
import * as Card from './style'
import useScrollIntoView from '../../useScrollIntoView'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import { OnReadFeedItem, OnSetPublisherPref } from '../../'
import PublisherMeta from '../PublisherMeta'

interface Props {
  content: BraveToday.Article[]
  publisher: BraveToday.Publisher
  articleToScrollTo?: BraveToday.FeedItem
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
}

type ListItemProps = {
  item: BraveToday.Article
  publisher: BraveToday.Publisher
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
  shouldScrollIntoView: boolean
}

function ListItem (props: ListItemProps) {
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView)
  const onClick = useReadArticleClickHandler(props.onReadFeedItem, {
    item: props.item
  })
  return (
    <Card.ListItem>
      <a onClick={onClick} href={props.item.url} ref={cardRef}>
        <Card.Heading>
          {props.item.title}
        </Card.Heading>
        <Card.Time>{props.item.relative_time}</Card.Time>
      </a>
    </Card.ListItem>
  )
}

export default function PublisherGroup (props: Props) {
  // No content no render®
  if (props.content.length < 3) {
    return null
  }
  return (
    <Card.OrderedList>
      {props.publisher &&
        <Card.ListTitle>
          <PublisherMeta
            publisher={props.publisher}
            title={true}
            onSetPublisherPref={props.onSetPublisherPref}
          />
        </Card.ListTitle>
      }
      <Card.List>
        {
          props.content.map((item, index) => {
            const shouldScrollTo = (
              !!props.articleToScrollTo &&
              props.articleToScrollTo.url === item.url
            )
            return <ListItem
              publisher={props.publisher}
              item={item}
              key={index}
              shouldScrollIntoView={shouldScrollTo}
              onReadFeedItem={props.onReadFeedItem}
              onSetPublisherPref={props.onSetPublisherPref}
            />
          })
        }
      </Card.List>
    </Card.OrderedList>
  )
}
