// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as BraveNews from '../../../../../api/brave_news'
import { CardImageFromFeedItem } from '../CardImage'
import * as Card from '../../cardSizes'
import PublisherMeta from '../PublisherMeta'
import useScrollIntoView from '../../useScrollIntoView'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import { OnReadFeedItem, OnSetPublisherPref } from '../../'
// TODO(petemill): Large and Medium article should be combined to 1 component.

interface Props {
  content: BraveNews.FeedItem[]
  publishers: BraveNews.Publishers
  articleToScrollTo?: BraveNews.FeedItemMetadata
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
}

type ArticleProps = {
  item: BraveNews.FeedItem
  publisher?: BraveNews.Publisher
  shouldScrollIntoView?: boolean
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
}

function MediumArticle (props: ArticleProps) {
  const { publisher, item } = props
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView || false)
  const onClick = useReadArticleClickHandler(props.onReadFeedItem, {
    item
  })
  const data = item.article?.data
  if (!data) {
    return null
  }
  return (
    <Card.Small data-score={data.score}>
      <a onClick={onClick} href={data.url.url} ref={cardRef}>
        <CardImageFromFeedItem
          data={data}
        />
        <Card.Content>
          <Card.Text>
            {data.title}
          <Card.Time>{data.relativeTimeDescription}</Card.Time>
          {
            publisher &&
              <Card.Publisher>
                <PublisherMeta
                  publisher={publisher}
                  onSetPublisherPref={props.onSetPublisherPref}
                />
              </Card.Publisher>
          }
          </Card.Text>
        </Card.Content>
      </a>
    </Card.Small>
  )
}

export default function CardSingleArticleMedium (props: Props) {
  const { content }: Props = props

  // no full content no render®
  if (content.length !== 2) {
    return null
  }

  return (
    <Card.ContainerForTwo>
      {
        content.map((item, index) => {
          const data = item.article?.data
          if (!data) {
            return (
              <React.Fragment key={index} />
            )
          }
          const shouldScrollIntoView = props.articleToScrollTo && (props.articleToScrollTo.url.url === data.url.url)
          const publisher = props.publishers[data.publisherId]
          return (
            <MediumArticle
              key={index}
              publisher={publisher}
              item={item}
              onReadFeedItem={props.onReadFeedItem}
              onSetPublisherPref={props.onSetPublisherPref}
              shouldScrollIntoView={shouldScrollIntoView}
            />
          )
        })
      }
    </Card.ContainerForTwo>
  )
}
