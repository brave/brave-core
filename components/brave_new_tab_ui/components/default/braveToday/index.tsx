// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as BraveTodayElement from './default'

// Feature Containers
import CardIntro from './cards/cardIntro'

const Content = React.lazy(() => import('./content'))

type State = {
  hasInteractionStarted: boolean
}

export type Props = {
  feed?: BraveToday.Feed
  publishers?: BraveToday.Publishers
  onInteracting: (interacting: boolean) => void
  onReadFeedItem: (item: BraveToday.FeedItem) => any
  onAnotherPageNeeded: () => any
  articleToScrollTo?: BraveToday.FeedItem
  displayedPageCount: number
}

class BraveToday extends React.PureComponent<Props, State> {
  braveTodayHitsViewportObserver: IntersectionObserver
  scrollTriggerToFocusBraveToday: any // React.RefObject<any>

  state = {
    hasInteractionStarted: false
  }

  componentDidMount () {
    const options = { root: null, rootMargin: '0px', threshold: 0.25 }

    this.braveTodayHitsViewportObserver = new
      IntersectionObserver(this.handleBraveTodayHitsViewportObserver.bind(this), options)

    // Handle first card showing up so we can hide secondary UI
    this.braveTodayHitsViewportObserver.observe(this.scrollTriggerToFocusBraveToday)
  }

  handleBraveTodayHitsViewportObserver (entities: IntersectionObserverEntry) {
    const isIntersecting = entities[0].isIntersecting
    this.props.onInteracting(isIntersecting)
    if (isIntersecting) {
      this.setState({ hasInteractionStarted: true })
    }
  }

  render () {
    const shouldDisplayContent =
      this.state.hasInteractionStarted ||
      !!this.props.articleToScrollTo

    return (
      <BraveTodayElement.Section>
        <div
          ref={scrollTrigger => (this.scrollTriggerToFocusBraveToday = scrollTrigger)}
          style={{ position: 'sticky', top: '100px' }}
        />

        <CardIntro />

        { shouldDisplayContent &&
        <React.Suspense fallback={(<div>Loading...</div>)}>
          <Content {...this.props} />
        </React.Suspense>
        }

      </BraveTodayElement.Section>
    )
  }
}

export default BraveToday
