// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Card from '../../cardSizes'
import CardImage from '../CardImage'
import PublisherMeta from '../PublisherMeta'
import useScrollIntoView from '../../useScrollIntoView'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import { OnReadFeedItem } from '../../'
// TODO(petemill): Large and Medium article should be combined to 1 component.

interface Props {
  content: (BraveToday.Article | undefined)[]
  publishers: BraveToday.Publishers
  articleToScrollTo?: BraveToday.FeedItem
  onReadFeedItem: OnReadFeedItem
}

type ArticleProps = {
  item: BraveToday.Article
  publisher?: BraveToday.Publisher
  shouldScrollIntoView?: boolean
  onReadFeedItem: OnReadFeedItem
}

const LargeArticle = React.forwardRef<HTMLElement, ArticleProps>(function (props: ArticleProps, ref) {
  const { publisher, item } = props
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView || false)
  const onClick = useReadArticleClickHandler(props.onReadFeedItem, item)
  // `ref as any` due to https://github.com/DefinitelyTyped/DefinitelyTyped/issues/28884
  return (
    <Card.Large innerRef={ref as any}>
      <a onClick={onClick} href={item.url} ref={cardRef}>
        <CardImage
          imageUrl={item.img}
        />
        <Card.Content>
          <Card.Heading>
            {item.title}
          </Card.Heading>
          <Card.Time>{item.relative_time}</Card.Time>
          {
            publisher &&
              <PublisherMeta publisher={publisher} />
          }
        </Card.Content>
      </a>
    </Card.Large>
  )
})

const CardSingleArticleLarge = React.forwardRef<HTMLElement, Props>(function (props, ref) {
  // no full content no render®
  if (props.content.length === 0) {
    return <></>
  }

  return (
    <>
      {props.content.map((item, index) => {
        // If there is a missing item, return nothing
        if (item === undefined) {
          return <></>
        }

        const shouldScrollIntoView = props.articleToScrollTo && (props.articleToScrollTo.url === item.url)

        const publisher = props.publishers[item.publisher_id]

        return (
          <LargeArticle
            ref={ref}
            key={`card-key-${index}`}
            publisher={publisher}
            item={item}
            shouldScrollIntoView={shouldScrollIntoView}
            onReadFeedItem={props.onReadFeedItem}
          />
        )
      })}
    </>
  )
})

export default CardSingleArticleLarge
