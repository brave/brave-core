// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as BraveNews from '../../../../../api/brave_news'
import * as Card from './style'
import useScrollIntoView from '../../useScrollIntoView'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import { OnReadFeedItem, OnSetPublisherPref } from '../../'
import PublisherMeta from '../PublisherMeta'

interface Props {
  content: (BraveNews.FeedItem)[]
  articleToScrollTo?: BraveNews.FeedItemMetadata
  publisher: BraveNews.Publisher
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
}

type ListItemProps = {
  item: BraveNews.FeedItem
  publisher: BraveNews.Publisher
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
  shouldScrollIntoView: boolean
}

function ListItem (props: ListItemProps) {
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView)
  const onClick = useReadArticleClickHandler(props.onReadFeedItem, {
    item: props.item
  })
  const data = props.item.article?.data
  if (!data) {
    return null
  }
  return (
    <Card.ListItem>
      <a onClick={onClick} href={data.url.url} ref={cardRef}>
        <Card.Heading>
          {data.title}
        </Card.Heading>
        <Card.Time>{data.relativeTimeDescription}</Card.Time>
      </a>
    </Card.ListItem>
  )
}

export default function PublisherGroup (props: Props) {
  // No content no render®
  if (props.content.length < 3 || props.content.some(c => !c.article)) {
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
            const data = item.article?.data
            // we already validated this, but typescript wants
            // us to do it again
            if (!data) {
              return <React.Fragment key={index} />
            }
            const shouldScrollTo = (
              !!props.articleToScrollTo &&
              props.articleToScrollTo.url.url === data.url.url
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
