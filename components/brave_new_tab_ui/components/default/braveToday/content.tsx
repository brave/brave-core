// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import CardLarge from './cards/_articles/cardArticleLarge'
import CardDeals from './cards/cardDeals'
import CardsGroup from './cardsGroup'
import { Props } from './'

export default function BraveTodayContent (props: Props) {
  const { feed, publishers } = props

  // const scrollTriggerToLoadMorePosts = React.useRef<HTMLDivElement>(null)
  const previousYAxis = React.useRef(0)

  const setScrollTriggerRef = React.useCallback((element) => {
    if (!element) {
      return
    }
    const options = { root: null, rootMargin: '0px', threshold: 1.0 }
    const endOfCurrentArticlesListObserver = new IntersectionObserver((entries: IntersectionObserverEntry[]) => {
      const currentYAxisForPostsBottom = entries[0].boundingClientRect.top
      if (previousYAxis.current > currentYAxisForPostsBottom) {
        props.onAnotherPageNeeded()
      }
      previousYAxis.current = currentYAxisForPostsBottom
    }, options)
    // Load up more posts (infinite scroll)
    endOfCurrentArticlesListObserver.observe(element)
  }, [previousYAxis])

  if (!feed) {
    return null
  }
  if (!publishers) {
    return null
  }

  const displayedPageCount = Math.min(props.displayedPageCount, feed.pages.length)

  return (
    <>
    {/* featured item */}
      <CardLarge
        content={[feed.featuredArticle]}
        publishers={publishers}
        articleToScrollTo={props.articleToScrollTo}
        onReadFeedItem={props.onReadFeedItem}
      />
      {/* deals */}
      { feed.featuredDeals && <CardDeals content={feed.featuredDeals} /> }
      {
        /* Infinitely repeating collections of content. */
        Array(displayedPageCount).fill(undefined).map((_: undefined, index: number) => {
          return (
            <CardsGroup
              key={index}
              content={feed.pages[index]}
              publishers={publishers}
              articleToScrollTo={props.articleToScrollTo}
              onReadFeedItem={props.onReadFeedItem}
            />
          )
        })
      }
      <div ref={setScrollTriggerRef} />
    </>
  )
}
