// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import * as BraveNews from '../../../api/brave_news'
import * as TodayActions from '../../../actions/today_actions'
import * as BraveTodayElement from './default'
import CardOptIn from './cards/cardOptIn'
import CardLoading from './cards/cardLoading'
const Content = React.lazy(() => import('./content'))

export type OnReadFeedItem = (args: TodayActions.ReadFeedItemPayload) => any
export type OnSetPublisherPref = (publisherId: string, enabled: boolean) => any
export type OnPromotedItemViewed = (args: TodayActions.PromotedItemViewedPayload) => any
export type OnVisitDisplayAd = (args: TodayActions.VisitDisplayAdPayload) => any
export type OnViewedDisplayAd = (args: TodayActions.DisplayAdViewedPayload) => any
export type GetDisplayAdContent = BraveNews.BraveNewsControllerRemote['getDisplayAd']

export type Props = {
  isFetching: boolean
  hasInteracted: boolean
  isUpdateAvailable: boolean
  isOptedIn: boolean
  isPrompting: boolean
  feed?: BraveNews.Feed
  publishers?: BraveNews.Publishers
  articleToScrollTo?: BraveNews.FeedItemMetadata
  displayAdToScrollTo?: number
  displayedPageCount: number
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
  const dispatch = useDispatch()

  // Don't ask for initial data more than once
  const hasRequestedLoad = React.useRef(false)

  const loadDataObserver = React.useMemo<IntersectionObserver>(() => {
    const handleHits: IntersectionObserverCallback = (entries) => {
      // When the scroll trigger hits the viewport (or is above the viewport),
      // we can start to fetch data.
      const isIntersecting = entries.some(entry => entry.isIntersecting)
      if (isIntersecting && !hasRequestedLoad.current) {
        // Only send interaction event when we're not loading data simply because
        // we're promoting items. In that case, the interaction event will
        // be sent by the content component when the content is first interacted
        // with.
        const shouldMarkInteraction = !props.isPrompting
        console.debug('Brave News: section is in position that requires requesting data load')
        dispatch(TodayActions.refresh({ isFirstInteraction: shouldMarkInteraction }))
        hasRequestedLoad.current = true
      }
    }
    // Setup intersection observer params
    console.log('setting today viewport observer, should only happen once')
    return new IntersectionObserver(handleHits, intersectionOptions)
  }, [props.isPrompting])

  const loadDataTrigger = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    // When we have an element to observe, set it as the target.
    // Don't do anything if we don't need data.
    if (!loadDataTrigger.current || !props.isOptedIn || !!props.feed) {
      return
    }
    loadDataObserver.observe(loadDataTrigger.current)
    return () => {
      // Cleanup current observer if we get a new observer, or a new element to observe
      loadDataObserver.disconnect()
    }
  }, [loadDataObserver, loadDataTrigger.current, props.isOptedIn, !!props.feed])

  // Only load all the content DOM elements if we're
  // scrolled far down enough, otherwise it's too easy to scroll down
  // by accident and get all the elements added.
  // Also sanity check isOptedIn, but without it there shouldn't be any content
  // anyway.
  const shouldDisplayContent = props.isOptedIn &&
    (props.hasInteracted || props.isPrompting)
  console.log('Brave News', { hasInteracted: props.hasInteracted, isPrompt: props.isPrompting })

  return (
    <BraveTodayElement.Section>
      <div
        ref={loadDataTrigger}
        style={{ position: 'sticky', top: '100px' }}
      />
      { !props.isOptedIn &&
      <>
        <CardOptIn onOptIn={props.onOptIn} onDisable={props.onDisable} />
      </>
      }
      { shouldDisplayContent &&
      <React.Suspense fallback={(<CardLoading />)}>
        <Content {...props} />
      </React.Suspense>
      }

    </BraveTodayElement.Section>
  )
}
