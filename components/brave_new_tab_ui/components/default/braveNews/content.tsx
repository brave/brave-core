// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import * as TodayActions from '../../../actions/today_actions'
import { Feed } from '../../../../brave_news/browser/resources/shared/api'
import CardLoading from './cards/cardLoading'
import CardError from './cards/cardError'
import CardNoContent from './cards/cardNoContent'
import CardLarge from './cards/_articles/cardArticleLarge'
import CardDisplayAd from './cards/displayAd'
import CardsGroup from './cardsGroup'
import Customize from './options/customize'
import { attributeNameCardCount, Props } from './'
import Refresh from './options/refresh'
import { useBraveNews } from '../../../../brave_news/browser/resources/shared/Context'

function getFeedHashForCache (feed?: Feed) {
  return feed ? feed.hash : ''
}

let pageRequestPending = false

export default function BraveNewsContent (props: Props) {
  const { feed, publishers } = props

  const dispatch = useDispatch()
  const { setCustomizePage } = useBraveNews()

  const previousYAxis = React.useRef(0)
  const [showOptions, setShowOptions] = React.useState(false)

  // When an element at the bottom enters the viewport, ask for a new page
  const setScrollTriggerRef = React.useCallback((element) => {
    if (!element) {
      return
    }
    // Trigger when it's 900px from viewport bottom so it's more seamless
    const intersectionOptions: IntersectionObserverInit = {
      rootMargin: '0px 0px 900px 0px'
    }
    const endOfCurrentArticlesListObserver = new IntersectionObserver((entries) => {
      console.debug('Brave News content Intersection Observer triggered', entries)
      const hasReachedPaginationPoint = entries.some(
        entry => entry.intersectionRatio > 0 ||
                  entry.boundingClientRect.top < 0)
      if (hasReachedPaginationPoint) {
        console.debug('Brave News content Intersection Observer determined need new page.')
        if (!pageRequestPending) {
          pageRequestPending = true
          window.requestIdleCallback(() => {
            pageRequestPending = false
            props.onAnotherPageNeeded()
          })
        }
      }
    }, intersectionOptions)
    // Load up more posts (infinite scroll)
    endOfCurrentArticlesListObserver.observe(element)
  }, [previousYAxis])

  // Show the options buttons when we enter the viewport
  const hasMarkedInteractionBegin = React.useRef(false)
  const interactionTriggerElement = React.useRef<HTMLDivElement>(null)
  const interactionObserver = React.useMemo<IntersectionObserver>(() => {
    const observer = new IntersectionObserver((entries) => {
      // Show if target article is inside or above viewport.
      const isInteracting = entries.some(
        entry => entry.isIntersecting ||
          entry.boundingClientRect.top < 0
      )
      console.debug('Brave News: Intersection Observer trigger show options, changing', isInteracting)
      setShowOptions(isInteracting)
      // When in "prompting" mode, scrolling past the first card
      // is indication that the user is interacting with the feature.
      if (isInteracting && props.isPrompting && !hasMarkedInteractionBegin.current) {
        dispatch(TodayActions.interactionBegin())
        hasMarkedInteractionBegin.current = true
      }
    })
    return observer
  }, [props.isPrompting])

  React.useEffect(() => {
    if (!interactionTriggerElement.current) {
      return
    }
    const target = interactionTriggerElement.current
    interactionObserver.observe(target)
    return () => {
      interactionObserver.unobserve(target)
    }
  }, [interactionObserver, interactionTriggerElement.current])

  // When the feed is refreshed, scroll to the top
  const prevFeedHashRef = React.useRef(getFeedHashForCache(props.feed))
  React.useEffect(() => {
    const currentFeedHash = getFeedHashForCache(props.feed)
    if (prevFeedHashRef.current && prevFeedHashRef.current !== currentFeedHash) {
      // Feed hash changed, make sure we scroll to top
      if (interactionTriggerElement.current) {
        interactionTriggerElement.current.scrollIntoView({
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

  // Report on how many cards have been viewed periodically
  // (every 4 cards).
  const latestCardDepth = React.useRef(0)
  const intersectionObserver = React.useRef(new IntersectionObserver(entries => {
    const inView = entries.filter(e => e.intersectionRatio > 0)
    if (!inView.length) {
      return
    }
    const counts = inView.map(i => Number(i.target.getAttribute(attributeNameCardCount)))
    counts.sort((a, b) => b - a)
    const count = counts[0]
    // Handle not deeper than we've previously been this session
    if (latestCardDepth.current >= count) {
      return
    }
    console.debug(`Brave News: viewed ${count} cards.`)
    props.onFeedItemViewedCountChanged(count)
  }))
  const registerCardCountTriggerElement = React.useCallback((trigger: HTMLElement | null) => {
    if (!trigger) {
      return
    }
    intersectionObserver.current.observe(trigger)
  }, [intersectionObserver.current])

  // TODO(petemill): Only way to test for error state is if there are no
  // publishers since GetFeed and GetPublishers will always eventually return
  // empty objects. Consider having those mojom functions provide error states.
  const hasContent = feed && publishers && !!Object.keys(publishers).length

  // Loading state
  if (props.isFetching && !hasContent) {
    return <CardLoading />
  }

  // Error state
  if (!hasContent) {
    return <CardError onRefresh={props.onRefresh} />
  }

  // satisfy typescript sanity, should not get here
  if (!feed || !publishers) {
    console.error('Brave News: should have shown error or loading state, but ran in to an unintended code path.')
    return null
  }

  const isOnlyDisplayingPeekingCard = !props.hasInteracted && props.isPrompting
  const displayedPageCount = Math.min(props.displayedPageCount, feed.pages.length)
  const introCount = feed.featuredItem ? 2 : 1
  const showNoContentMessage = !props.isFetching &&
    hasContent &&
    !feed.featuredItem &&
    feed.pages.length === 0
  let runningCardCount = introCount
  return (
    <>
      {/* no feed content available */}
      {showNoContentMessage &&
      <CardNoContent onCustomize={props.onCustomizeBraveNews} />}
      {/* featured item */}
      {feed.featuredItem && <CardLarge
        content={[feed.featuredItem]}
        publishers={publishers}
        articleToScrollTo={props.articleToScrollTo}
        onSetPublisherPref={props.onSetPublisherPref}
        onReadFeedItem={props.onReadFeedItem}
      />
      }
      <div ref={interactionTriggerElement} />
      <div {...{ [attributeNameCardCount]: 1 }} ref={registerCardCountTriggerElement} />
      {!isOnlyDisplayingPeekingCard &&
        <>
          <>
            <CardDisplayAd
              onVisitDisplayAd={props.onVisitDisplayAd}
              onViewedDisplayAd={props.onViewedDisplayAd}
              getContent={props.getDisplayAd}
            />
            <div {...{ [attributeNameCardCount]: introCount }} ref={registerCardCountTriggerElement} />
          </>
          {
            /* Infinitely repeating collections of content. */
            Array(displayedPageCount).fill(undefined).map((_: undefined, index: number) => {
              const shouldScrollToDisplayAd = props.displayAdToScrollTo === (index + 1)
              let startingDisplayIndex = runningCardCount
              runningCardCount += feed.pages[index].items.length
              return (
                <CardsGroup
                  key={index}
                  itemStartingDisplayIndex={startingDisplayIndex}
                  content={feed.pages[index]}
                  publishers={publishers}
                  articleToScrollTo={props.articleToScrollTo}
                  shouldScrollToDisplayAd={shouldScrollToDisplayAd}
                  onReadFeedItem={props.onReadFeedItem}
                  onPeriodicCardViews={registerCardCountTriggerElement}
                  onSetPublisherPref={props.onSetPublisherPref}
                  onPromotedItemViewed={props.onPromotedItemViewed}
                  onVisitDisplayAd={props.onVisitDisplayAd}
                  onViewedDisplayAd={props.onViewedDisplayAd}
                  getDisplayAdContent={props.getDisplayAd}
                />
              )
            })
          }
          <Customize onCustomizeBraveNews={() => setCustomizePage('news')} show={showOptions} />
          <Refresh isFetching={props.isFetching} show={showOptions && (props.isUpdateAvailable || props.isFetching)} onClick={props.onRefresh} />
          <div
            ref={setScrollTriggerRef}
            style={{
              width: '1px',
              height: '1px',
              position: 'absolute',
              bottom: '0'
            }}
          />
        </>
      }
    </>
  )
}
