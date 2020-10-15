// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as BraveTodayElement from './default'

// Feature Containers
import CardIntro from './cards/cardIntro'
import CardLarge from './cards/_articles/cardArticleLarge'
import CardDeals from './cards/cardDeals'
import CardsGroup from './cardsGroup'

interface State {
  contentPage: number
  previousYAxis: number
}

interface Props {
  feed?: BraveToday.Feed
  publishers?: BraveToday.Publishers
  setOpacityForItems: (opacity: boolean) => void
  onAnotherPageNeeded: () => any
  displayedPageCount: number
}

class BraveToday extends React.PureComponent<Props, State> {
  braveTodayHitsViewportObserver: IntersectionObserver
  endOfCurrentArticlesListObserver: IntersectionObserver
  scrollTriggerToLoadMorePosts: any // React.RefObject<any>
  scrollTriggerToFocusBraveToday: any // React.RefObject<any>
  previousYAxis: number

  constructor (props: Props) {
    super(props)
    // this.scrollTriggerToLoadMorePosts = React.createRef()
    this.state = {
      contentPage: 1,
      previousYAxis: 0
    }
  }

  componentDidMount () {
    const { contentPage } = this.state
    this.setState({ contentPage })

    const options = { root: null, rootMargin: '0px', threshold: 0.25 }
    const options2 = { ...options, threshold: 1.0 }

    this.braveTodayHitsViewportObserver = new
      IntersectionObserver(this.handleBraveTodayHitsViewportObserver.bind(this), options)

    this.endOfCurrentArticlesListObserver = new
      IntersectionObserver(this.handleEndOfCurrentArticlesListObserver.bind(this), options2)

    // Handle first card showing up so we can hide secondary UI
    this.braveTodayHitsViewportObserver.observe(this.scrollTriggerToFocusBraveToday)

    // Load up more posts (infinite scroll)
    this.endOfCurrentArticlesListObserver.observe(this.scrollTriggerToLoadMorePosts)
  }

  handleBraveTodayHitsViewportObserver (entities: IntersectionObserverEntry) {
    const isIntersecting = entities[0].isIntersecting
    this.props.setOpacityForItems(isIntersecting)
  }

  handleEndOfCurrentArticlesListObserver (entities: IntersectionObserverEntry) {

    const currentYAxisForPostsBottom = entities[0].boundingClientRect.y

    if (this.previousYAxis > currentYAxisForPostsBottom) {
      this.props.onAnotherPageNeeded()
    }

    this.previousYAxis = currentYAxisForPostsBottom
  }

  render () {
    const { feed, publishers } = this.props

    if (!feed) {
      // TODO: loading state
      console.warn('today: no feed yet')
      return null
    }

    if (!publishers) {
      console.warn('today: no publishers yet')
      return null
    }

    const displayedPageCount = Math.min(this.props.displayedPageCount, feed.pages.length)

    return (
      <BraveTodayElement.Section>
        <div
          ref={scrollTrigger => (this.scrollTriggerToFocusBraveToday = scrollTrigger)}
          style={{ position: 'sticky', top: '100px' }}
        />
        <CardIntro />
        {/* featured item */}
        <CardLarge
          content={[feed.featuredArticle]}
          publishers={publishers}
        />
        {/* deals */}
        <CardDeals content={feed.featuredDeals} />
        {
          /* Infinitely repeating collections of content. */
          Array(displayedPageCount).fill(undefined).map((_: undefined, index: number) => {
            return (
              <>
                <CardsGroup
                  key={`cards-group-key-${index}`}
                  content={feed.pages[index]}
                  publishers={publishers}
                />
              </>
            )
          })
        }
        <div ref={scrollTrigger => (this.scrollTriggerToLoadMorePosts = scrollTrigger)} />
      </BraveTodayElement.Section>
    )
  }
}

export default BraveToday
