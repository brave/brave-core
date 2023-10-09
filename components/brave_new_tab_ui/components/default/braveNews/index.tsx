// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import * as BraveNews from '../../../../brave_news/browser/resources/shared/api'
import * as TodayActions from '../../../actions/today_actions'
import * as BraveNewsElement from './default'
import CardOptIn from './cards/cardOptIn'
import CardLoading from './cards/cardLoading'
import { useNewTabPref } from '../../../hooks/usePref'
import { defaultState } from '../../../storage/new_tab_storage'
import { FeedV2 } from './FeedV2'
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
  onCustomizeBraveNews: () => any
  onRefresh: () => any
  onCheckForUpdate: () => any
  getDisplayAd: GetDisplayAdContent
}

type PossibleInteractionObserver = IntersectionObserver | undefined

export const attributeNameCardCount = 'data-today-card-count'

const intersectionOptions = { root: null, rootMargin: '0px', threshold: 0.25 }

export default function BraveNewsSection(props: Props) {
  const dispatch = useDispatch()
  const [optedIn, setOptedIn] = useNewTabPref('isBraveNewsOptedIn')
  const [, setShowToday] = useNewTabPref('showToday')

  // Don't ask for initial data more than once
  const hasRequestedLoad = React.useRef(false)

  const loadDataObserver = React.useMemo<PossibleInteractionObserver>(() => {
    // Handle case where we're already told to load content, e.g. when
    // navigating back to a position in the feed after clicking an article
    // or ad.
    if (props.hasInteracted) {
      dispatch(TodayActions.refresh({ isFirstInteraction: false }))
      hasRequestedLoad.current = true
      return
    }
    // Handle case where we wait for News content area to hit viewport
    // before lazy-loading content.
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
    return new IntersectionObserver(handleHits, intersectionOptions)
  }, [props.isPrompting, props.hasInteracted])

  const loadDataTrigger = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    // When we have an element to observe, set it as the target.
    // Don't do anything if we don't need data.
    if (!loadDataObserver ||
      !loadDataTrigger.current ||
      !optedIn ||
      !!props.feed) {
      return
    }
    loadDataObserver.observe(loadDataTrigger.current)
    return () => {
      // Cleanup current observer if we get a new observer, or a new element to observe
      loadDataObserver.disconnect()
    }
  }, [loadDataObserver, loadDataTrigger.current, optedIn, !!props.feed])

  // Only load all the content DOM elements if we're
  // scrolled far down enough, otherwise it's too easy to scroll down
  // by accident and get all the elements added.
  // Also sanity check isOptedIn, but without it there shouldn't be any content
  // anyway.
  const shouldDisplayContent = optedIn &&
    (props.hasInteracted || props.isPrompting)

  return (
    <BraveNewsElement.Section>
      <div
        ref={loadDataTrigger}
        style={{ position: 'sticky', top: '100px' }}
      />
      {!optedIn &&
        <>
          <CardOptIn onOptIn={() => setOptedIn(true)} onDisable={() => setShowToday(false)} />
        </>
      }
      {shouldDisplayContent && (defaultState.featureFlagBraveNewsFeedV2Enabled
        ? <FeedV2 />
        : <React.Suspense fallback={(<CardLoading />)}>
          <Content {...props} />
        </React.Suspense>)
      }

    </BraveNewsElement.Section>
  )
}
