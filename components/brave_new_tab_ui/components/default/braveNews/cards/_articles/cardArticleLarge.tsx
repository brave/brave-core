// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { OnReadFeedItem, OnSetPublisherPref } from '../../'
import * as BraveNews from '../../../../../../brave_news/browser/resources/shared/api'
import * as Card from '../../cardSizes'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import useScrollIntoView from '../../useScrollIntoView'
import { CardImageFromFeedItem } from '../CardImage'
import PublisherMeta from '../PublisherMeta'
// TODO(petemill): Large and Medium article should be combined to 1 component.

type Props = {
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
}

type ArticlesProps = Props & {
  content: BraveNews.FeedItem[]
  publishers: BraveNews.Publishers
  articleToScrollTo?: BraveNews.FeedItemMetadata
}

type ArticleProps = Props & {
  item: BraveNews.FeedItem
  publisher?: BraveNews.Publisher
  shouldScrollIntoView?: boolean
}

const LargeArticle = React.forwardRef<HTMLElement, ArticleProps>(function (props: ArticleProps, forwardedRef) {
  const { publisher, item } = props
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView || false)

  const innerRef = React.useRef<HTMLElement>(null)

  const data = item.article?.data

  const onClick = useReadArticleClickHandler(props.onReadFeedItem, { item })

  React.useEffect(() => {
    if (!innerRef.current) {
      return
    }
    // Pass ref to parent
    if (forwardedRef) {
      if (typeof forwardedRef === 'function') {
        forwardedRef(innerRef.current)
      } else {
        // @ts-expect-error
        // Ref.current is meant to be readonly, but we can ignore that.
        ref.current = newRef
      }
    }
  }, [innerRef.current])

  if (!data) {
    return null
  }

  // TODO(petemill): Avoid nested links
  // `ref as any` due to https://github.com/DefinitelyTyped/DefinitelyTyped/issues/28884
  return (
    <Card.Large data-score={data.score} ref={innerRef}>
      <a onClick={onClick} href={data.url.url} ref={cardRef}>
        <CardImageFromFeedItem
          data={data}
        />
        <Card.Content>
          <Card.Heading>
            {data.title}
          </Card.Heading>
          <Card.Time>{data.relativeTimeDescription}</Card.Time>
          {
            publisher &&
            <Card.Source>
              <Card.Publisher>
                <PublisherMeta
                  publisher={publisher}
                  onSetPublisherPref={props.onSetPublisherPref}
                />
              </Card.Publisher>
            </Card.Source>
          }
        </Card.Content>
      </a>
    </Card.Large>
  )
})

const CardSingleArticleLarge = React.forwardRef<HTMLElement, ArticlesProps>(function (props, ref) {
  // no full content no render®
  if (props.content.length === 0) {
    return null
  }

  return (
    <>
      {props.content.map((item, index) => {
        const key = `card-key-${index}`
        const data = item.article?.data
        // If there is a missing item, return nothing
        if (!data) {
          return (
            <React.Fragment
              key={key}
            />
          )
        }

        const shouldScrollIntoView = (props.articleToScrollTo &&
            (props.articleToScrollTo.url.url === data.url.url))

        const publisher = props.publishers[data.publisherId]

        return (
          <LargeArticle
            ref={ref}
            key={key}
            publisher={publisher}
            item={item}
            shouldScrollIntoView={shouldScrollIntoView}
            onReadFeedItem={props.onReadFeedItem}
            onSetPublisherPref={props.onSetPublisherPref}
          />
        )
      })}
    </>
  )
})

export default CardSingleArticleLarge
