// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import CardLoading from './cards/cardLoading'
import CardError from './cards/cardError'
import CardLarge from './cards/_articles/cardArticleLarge'
import CardDeals from './cards/cardDeals'
import CardsGroup from './cardsGroup'
import Customize from './options/customize'
import { Props } from './'
import Refresh from './options/refresh'

function getFeedHashForCache (feed?: BraveToday.Feed) {
  return feed ? feed.hash : ''
}

let pageRequestPending = false

export default function BraveTodayContent (props: Props) {
  const { feed, publishers } = props

  const previousYAxis = React.useRef(0)
  const [showOptions, setShowOptions] = React.useState(false)

  // When an element at the bottom enters the viewport, ask for a new page
  const setScrollTriggerRef = React.useCallback((element) => {
    if (!element) {
      return
    }
    const endOfCurrentArticlesListObserver = new IntersectionObserver((entries) => {
      console.debug('Brave Today content Intersection Observer triggered')
      if (entries.some(entry => entry.intersectionRatio > 0)) {
        console.debug('Brave Today content Intersection Observer determined need new page.')
        if (!pageRequestPending) {
          pageRequestPending = true
          window.requestIdleCallback(() => {
            pageRequestPending = false
            props.onAnotherPageNeeded()
          })
        }
      }
    })
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
      console.debug('Intersection Observer trigger show options', [...entries])
      // Show if target article is inside or above viewport.
      const shouldShowOptions = entries.some(
        entry => entry.isIntersecting
                  || entry.boundingClientRect.top < 0
      )
      console.debug('Intersection Observer trigger show options, changing', shouldShowOptions)
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

  // Check for changes often, (won't incur remote check,
  // only local unless it's been awhile).
  React.useEffect(() => {
    const intervalMs = 2 * 60 * 1000
    let timeoutHandle: number
    setInterval(() => {
      props.onCheckForUpdate()
    }, intervalMs)
    // Cancel the timer when we are unmounted
    return () => {
      if (timeoutHandle) {
        clearInterval(timeoutHandle)
      }
    }
  }, [])

  const hasContent = feed && publishers
  // Loading state
  if (props.isFetching && !hasContent) {
    return <CardLoading />
  }

  // Error state
  if (!hasContent) {
    return <CardError />
  }

  // satisfy typescript sanity, should not get here
  if (!feed || !publishers) {
    console.error('Brave Today: should have shown error or loading state, but ran in to an unintended code path.')
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
        onSetPublisherPref={props.onSetPublisherPref}
        onReadFeedItem={props.onReadFeedItem}
      />
      {/* deals */}
      {feed.featuredDeals &&
      <CardDeals
        content={feed.featuredDeals}
        articleToScrollTo={props.articleToScrollTo}
        onReadFeedItem={props.onReadFeedItem}
      />
      }
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
              onSetPublisherPref={props.onSetPublisherPref}
            />
          )
        })
      }
      <Customize onCustomizeBraveToday={props.onCustomizeBraveToday} show={showOptions} />
      <Refresh isFetching={props.isFetching} show={showOptions && (props.isUpdateAvailable || props.isFetching)} onClick={props.onRefresh} />
      <div
        ref={setScrollTriggerRef}
        style={{
          width: '1px',
          height: '1px',
          position: 'absolute',
          bottom: '900px'
        }}
      />
    </>
  )
}
