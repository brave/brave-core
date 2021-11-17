// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as BraveNews from '../../../../../api/brave_news'
import VisibilityTimer from '../../../../../helpers/visibilityTimer'
import { getLocale } from '../../../../../../common/locale'
import * as Card from '../../cardSizes'
import useScrollIntoView from '../../useScrollIntoView'
import useReadArticleClickHandler from '../../useReadArticleClickHandler'
import { OnPromotedItemViewed, OnReadFeedItem, OnSetPublisherPref } from '../../'
import { CardImageFromFeedItem } from '../CardImage'
import PublisherMeta from '../PublisherMeta'
// TODO(petemill): Large and Medium article should be combined to 1 component.

type Props = {
  onReadFeedItem: OnReadFeedItem
  onSetPublisherPref: OnSetPublisherPref
  onItemViewed?: OnPromotedItemViewed
  isPromoted?: boolean
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

  const innerRef = React.useRef<HTMLElement>(null)

  const data = item.article?.data || item.promotedArticle?.data

  const uuid = React.useMemo<string | undefined>(function () {
    if (props.isPromoted) {
      // @ts-expect-error
      const uuid: string = crypto.randomUUID()
      return uuid
    }
    return undefined
  }, [props.isPromoted, data?.url.url])

  const onClick = useReadArticleClickHandler(props.onReadFeedItem, { item, isPromoted: props.isPromoted, promotedUUID: uuid })

  const onItemViewedRef = React.useRef<Function | null>()
  onItemViewedRef.current = props.onItemViewed
    ? props.onItemViewed.bind(undefined, { item: props.item, uuid })
    : null

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
    // If asked, detect when card is viewed, and send an action.
    if (!props.onItemViewed) {
      return
    }

    const observer = new VisibilityTimer(() => {
      const onItemViewed = onItemViewedRef.current
      if (onItemViewed) {
        onItemViewed()
      }
    }, 100, innerRef.current)

    observer.startTracking()

    return () => {
      observer.stopTracking()
    }
  }, [innerRef.current, Boolean(props.onItemViewed)])

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
          isPromoted={props.isPromoted}
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

const CardSingleArticleLarge = React.forwardRef<HTMLElement, ArticlesProps>(function (props, ref) {
  // no full content no render®
  if (props.content.length === 0) {
    return null
  }

  return (
    <>
      {props.content.map((item, index) => {
        const key = `card-key-${index}`
        const data = item.article?.data || item.promotedArticle?.data
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
            onItemViewed={props.onItemViewed}
            isPromoted={props.isPromoted}
          />
        )
      })}
    </>
  )
})

export default CardSingleArticleLarge
