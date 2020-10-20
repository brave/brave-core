// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react';
import CardImage from '../CardImage'
import * as Card from '../../cardSizes'
import PublisherMeta from '../PublisherMeta'

// TODO(petemill): Large and Medium article should be combined to 1 component.

interface Props {
  content: (BraveToday.Article)[]
  publishers: BraveToday.Publishers
  articleToScrollTo?: BraveToday.FeedItem
  onReadFeedItem: (item: BraveToday.FeedItem) => any
}

type ArticleProps = {
  item: BraveToday.Article
  publisher: BraveToday.Publisher
  shouldScrollIntoView?: boolean
  onReadFeedItem: (item: BraveToday.FeedItem) => any
}

function MediumArticle (props: ArticleProps) {
  const {publisher, item} = props
  // If we need to scroll the article in to view after render,
  // do so after the image has been loaded. Assume that all the other
  // previous images are loaded and the articles are occupying
  // the size they would do with images.
  const cardRef = React.useRef<HTMLAnchorElement>(null)
  let onImageLoaded = undefined
  if (props.shouldScrollIntoView) {
    const hasScrolled = React.useRef<boolean>(false)
    const hasImageLoaded = React.useRef<boolean>(false)
    const scrollIntoViewConditionally = function () {
      if (!hasScrolled.current && hasImageLoaded.current && cardRef.current) {
        hasScrolled.current = true
        cardRef.current.scrollIntoView({block: 'center'})
      }
    }
    onImageLoaded = () => {
      hasImageLoaded.current = true
      requestAnimationFrame(() => {
        scrollIntoViewConditionally()
      })
    }
    React.useEffect(() => {
      scrollIntoViewConditionally()
    }, []); // empty array so only runs on first render
  }

  return (
    <Card.Small>
      <a onClick={() => props.onReadFeedItem(item)} ref={cardRef}>
        <CardImage
          size='medium'
          imageUrl={item.img}
          onLoaded={onImageLoaded}
        />
        <Card.Content>
          <Card.Text>
            {item.title}
          <Card.Time>{item.relative_time}</Card.Time>
          {
            publisher &&
              <PublisherMeta publisher={publisher} />
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
          // If there is a missing item, return nothing
          if (item === undefined) {
            return null
          }
          const shouldScrollIntoView = props.articleToScrollTo && (props.articleToScrollTo.url === item.url)
          const publisher = props.publishers[item.publisher_id]
          return <MediumArticle
            key={index}
            publisher={publisher}
            item={item}
            onReadFeedItem={props.onReadFeedItem}
            shouldScrollIntoView={shouldScrollIntoView}
          />
        })
      }
    </Card.ContainerForTwo>
  )
}
