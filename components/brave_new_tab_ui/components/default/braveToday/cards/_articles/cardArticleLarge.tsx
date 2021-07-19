// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import VisibilityTimer from '../../../../../helpers/visibilityTimer'
import { getLocale } from '../../../../../../common/locale'
import * as Card from '../../cardSizes'
import useScrollIntoView from '../../useScrollIntoView'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import { OnReadFeedItem, OnSetPublisherPref } from '../../'
import CardImage from '../CardImage'
import PublisherMeta from '../PublisherMeta'
// TODO(petemill): Large and Medium article should be combined to 1 component.

interface Props {
  content: (BraveToday.Article | BraveToday.PromotedArticle | undefined)[]
  publishers: BraveToday.Publishers
  articleToScrollTo?: BraveToday.FeedItem
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
  onItemViewed?: (item: BraveToday.FeedItem) => any
  isPromoted?: boolean
}

type ArticleProps = {
  item: BraveToday.Article | BraveToday.PromotedArticle
  publisher?: BraveToday.Publisher
  shouldScrollIntoView?: boolean
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
  onItemViewed?: (item: BraveToday.FeedItem) => any
  isPromoted?: boolean
}

const promotedInfoUrl = 'https://brave.com/brave-today'

function onClickPromoted (e: React.MouseEvent) {
  const openInNewTab = e.ctrlKey || e.metaKey
  if (openInNewTab) {
    document.open(promotedInfoUrl, '__blank')
  } else {
    window.location.href = promotedInfoUrl
  }
  e.preventDefault()
}

const LargeArticle = React.forwardRef<HTMLElement, ArticleProps>(function (props: ArticleProps, forwardedRef) {
  const { publisher, item } = props
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView || false)

  const onClick = useReadArticleClickHandler(props.onReadFeedItem, { item, isPromoted: props.isPromoted })

  const innerRef = React.useRef<HTMLElement>(null)

  const onItemViewedRef = React.useRef(props.onItemViewed)
  onItemViewedRef.current = props.onItemViewed

  React.useEffect(() => {
    if (!innerRef.current) {
      return
    }
    // Pass ref to parent
    if (forwardedRef) {
      if (typeof forwardedRef === 'function') {
        forwardedRef(innerRef.current)
      } else {
        // @ts-ignore
        // Ref.current is meant to be readonly, but we can ignore that.
        ref.current = newRef
      }
    }
    // If asked, detect when card is viewed, and send an action.
    if (!props.onItemViewed) {
      return
    }

    const observer = new VisibilityTimer(() => {
      const onItemViewed = onItemViewedRef.current
      if (onItemViewed) {
        onItemViewed(item)
      }
    }, 100, innerRef.current)

    observer.startTracking()

    return () => {
      observer.stopTracking()
    }
  }, [innerRef.current, Boolean(props.onItemViewed)])

  // TODO(petemill): Avoid nested links
  // `ref as any` due to https://github.com/DefinitelyTyped/DefinitelyTyped/issues/28884
  return (
    <Card.Large ref={innerRef}>
      <a onClick={onClick} href={item.url} ref={cardRef}>
        <CardImage
          imageUrl={item.img}
          isPromoted={props.isPromoted}
        />
        <Card.Content>
          <Card.Heading>
            {item.title}
          </Card.Heading>
          <Card.Time>{item.relative_time}</Card.Time>
          {
            publisher &&
            <Card.Source>
              <Card.Publisher>
                <PublisherMeta
                  publisher={publisher}
                  onSetPublisherPref={props.onSetPublisherPref}
                />
              </Card.Publisher>
              {props.isPromoted &&
              <Card.PromotedLabel onClick={onClickPromoted} href={promotedInfoUrl}>
                <Card.PromotedIcon>
                  <svg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 16 9'>
                    <path
                      fill='#fff'
                      fillRule='evenodd'
                      d='M14.56 4.77a.9.9 0 01-.9-.9v-.83L8.73 7.27a.9.9 0 01-1.23-.05L5.36 5.08 1.47 8.19A.9.9 0 01.2 8.05a.9.9 0 01.14-1.27l4.52-3.62a.9.9 0 011.2.07L8.2 5.35l3.84-3.3h-.18a.9.9 0 110-1.8h2.71c.4 0 .67.26.77.62v.05c.02.08.05.15.05.23v2.72c0 .5-.32.9-.82.9z'
                      clipRule='evenodd'
                    />
                  </svg>
                </Card.PromotedIcon>
                {getLocale('promoted')}
              </Card.PromotedLabel>
              }
            </Card.Source>
          }
        </Card.Content>
      </a>
    </Card.Large>
  )
})

const CardSingleArticleLarge = React.forwardRef<HTMLElement, Props>(function (props, ref) {
  // no full content no render®
  if (props.content.length === 0) {
    return null
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
            onSetPublisherPref={props.onSetPublisherPref}
            onItemViewed={props.onItemViewed}
            isPromoted={props.isPromoted}
          />
        )
      })}
    </>
  )
})

export default CardSingleArticleLarge
