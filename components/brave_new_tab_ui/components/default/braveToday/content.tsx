// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import CardLarge from './cards/_articles/cardArticleLarge'
import CardDeals from './cards/cardDeals'
import CardsGroup from './cardsGroup'
import Customize from './options/customize'
import { Props } from './'
import Refresh from './options/refresh'

function getFeedHashForCache (feed?: BraveToday.Feed) {
  return feed ? feed.hash : ''
}

export default function BraveTodayContent (props: Props) {
  const { feed, publishers } = props

  const previousYAxis = React.useRef(0)
  const [showOptions, setShowOptions] = React.useState(false)

  // When an element at the bottom enters the viewport, ask for a new page
  const setScrollTriggerRef = React.useCallback((element) => {
    if (!element) {
      return
    }
    const options = { root: null, rootMargin: '0px', threshold: 1.0 }
    const endOfCurrentArticlesListObserver = new IntersectionObserver((entries) => {
      const currentYAxisForPostsBottom = entries[0].boundingClientRect.top
      if (previousYAxis.current > currentYAxisForPostsBottom) {
        props.onAnotherPageNeeded()
      }
      previousYAxis.current = currentYAxisForPostsBottom
    }, options)
    // Load up more posts (infinite scroll)
    endOfCurrentArticlesListObserver.observe(element)
  }, [previousYAxis])

  // Show the options buttons when we enter the viewport
  const optionsTriggerRef = React.useRef<HTMLElement>()
  const onOptionsTriggerElement = React.useCallback((element) => {
    if (!element) {
      return
    }
    optionsTriggerRef.current = element
    const observer = new IntersectionObserver((entries) => {
      const entry = entries[0]
      // Only concerned with whether we're below the
      // viewport threshold or not, not whether the target is out of viewport
      // because it's above viewport.
      const shouldShowOptions = entry.isIntersecting
        || entry.boundingClientRect.top < 0
      setShowOptions(shouldShowOptions)
    })
    observer.observe(element)
  }, [setShowOptions])

  // When the feed is refreshed, scroll to the top
  const prevFeedHashRef = React.useRef(getFeedHashForCache(props.feed))
  React.useEffect(() => {
    const currentFeedHash = getFeedHashForCache(props.feed)
    if (prevFeedHashRef.current && prevFeedHashRef.current !== currentFeedHash) {
      // Feed hash changed, make sure we scroll to top
      if (optionsTriggerRef.current) {
        optionsTriggerRef.current.scrollIntoView({
          behavior: 'smooth',
          block: 'center'
        })
      }
    }
    prevFeedHashRef.current = currentFeedHash
  })

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
        ref={onOptionsTriggerElement}
        content={[feed.featuredArticle]}
        publishers={publishers}
        articleToScrollTo={props.articleToScrollTo}
        onReadFeedItem={props.onReadFeedItem}
      />
      {/* deals */}
      {feed.featuredDeals && <CardDeals content={feed.featuredDeals} />}
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
      <Customize onCustomizeBraveToday={props.onCustomizeBraveToday} show={showOptions} />
      <Refresh isFetching={props.isFetching} show={showOptions && (props.isUpdateAvailable || props.isFetching)} onClick={props.onRefresh} />
      <div ref={setScrollTriggerRef} />
    </>
  )
}
