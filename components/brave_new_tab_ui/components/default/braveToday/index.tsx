// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as BraveNews from '../../../api/brave_news'
import { PromotedItemViewedPayload, ReadFeedItemPayload, DisplayAdViewedPayload, VisitDisplayAdPayload } from '../../../actions/today_actions'
import * as BraveTodayElement from './default'
import CardOptIn from './cards/cardOptIn'
import CardLoading from './cards/cardLoading'
const Content = React.lazy(() => import('./content'))

export type OnReadFeedItem = (args: ReadFeedItemPayload) => any
export type OnSetPublisherPref = (publisherId: string, enabled: boolean) => any
export type OnPromotedItemViewed = (args: PromotedItemViewedPayload) => any
export type OnVisitDisplayAd = (args: VisitDisplayAdPayload) => any
export type OnViewedDisplayAd = (args: DisplayAdViewedPayload) => any
export type GetDisplayAdContent = BraveNews.BraveNewsControllerRemote['getDisplayAd']

export type Props = {
  isFetching: boolean
  hasInteracted: boolean
  isUpdateAvailable: boolean
  isOptedIn: boolean
  feed?: BraveNews.Feed
  publishers?: BraveNews.Publishers
  articleToScrollTo?: BraveNews.FeedItemMetadata
  displayAdToScrollTo?: number
  displayedPageCount: number
  onInteracting: () => any
  onReadFeedItem: OnReadFeedItem
  onPromotedItemViewed: OnPromotedItemViewed
  onVisitDisplayAd: OnVisitDisplayAd
  onViewedDisplayAd: OnViewedDisplayAd
  onFeedItemViewedCountChanged: (feedItemsViewed: number) => any
  onSetPublisherPref: OnSetPublisherPref
  onAnotherPageNeeded: () => any
  onCustomizeBraveToday: () => any
  onRefresh: () => any
  onCheckForUpdate: () => any
  onOptIn: () => any
  onDisable: () => unknown
  getDisplayAd: GetDisplayAdContent
}

export const attributeNameCardCount = 'data-today-card-count'

const intersectionOptions = { root: null, rootMargin: '0px', threshold: 0.25 }

export default function BraveTodaySection (props: Props) {
  const handleHitsViewportObserver = React.useCallback<IntersectionObserverCallback>((entries) => {
    // When the scroll trigger, hits the viewport, notify externally, and since
    // we won't get updated with that result, change our internal state.
    const isIntersecting = entries.some(entry => entry.isIntersecting)
    if (isIntersecting) {
      props.onInteracting()
    }
  }, [props.onInteracting])

  const viewportObserver = React.useRef<IntersectionObserver>()
  React.useEffect(() => {
    // Setup intersection observer params
    console.log('setting today viewport observer, should only happen once')
    viewportObserver.current = new IntersectionObserver(handleHitsViewportObserver, intersectionOptions)
  }, [handleHitsViewportObserver])

  const scrollTrigger = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    // When we have an element to observe, set it as the target
    const observer = viewportObserver.current
    // Don't do anything if we're still setting up, or if we've already
    // observed and set `hasInteracted`.
    if (!observer || !scrollTrigger.current || !props.isOptedIn || props.hasInteracted) {
      return
    }
    observer.observe(scrollTrigger.current)
    return () => {
      // Cleanup current observer if we get a new observer, or a new element to observe
      observer.disconnect()
    }
  }, [scrollTrigger.current, viewportObserver.current, props.isOptedIn, props.hasInteracted])

  // Only load all the content DOM elements if we're
  // scrolled far down enough, otherwise it's too easy to scroll down
  // by accident and get all the elements added.
  // Also sanity check isOptedIn, but without it there shouldn't be any content
  // anyway.
  const shouldDisplayContent = props.isOptedIn &&
    (props.hasInteracted || !!props.articleToScrollTo)

  return (
    <BraveTodayElement.Section>
      <div
        ref={scrollTrigger}
        style={{ position: 'sticky', top: '100px' }}
      />
      { !props.isOptedIn &&
      <CardOptIn onOptIn={props.onOptIn} onDisable={props.onDisable} />
      }
      { shouldDisplayContent &&
      <React.Suspense fallback={(<CardLoading />)}>
        <Content {...props} />
      </React.Suspense>
      }

    </BraveTodayElement.Section>
  )
}
