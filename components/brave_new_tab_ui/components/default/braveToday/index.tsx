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
  setOpacityForItems: (opacity: boolean) => void
}

class BraveToday extends React.PureComponent<Props, State> {
  braveTodayHitsViewportObserver: IntersectionObserver
  endOfCurrentArticlesListObserver: IntersectionObserver
  scrollTriggerToLoadMorePosts: any // React.RefObject<any>
  scrollTriggerToFocusBraveToday: any // React.RefObject<any>

  constructor (props: Props) {
    super(props)
    // this.scrollTriggerToLoadMorePosts = React.createRef()
    this.state = {
      contentPage: 1,
      previousYAxis: 0
    }
  }

  get featuredPublisher (): BraveToday.OneOfPublishers {
    return "http://www.cbssports.com/partners/feeds/rss/home_news"
  }

  get featuredCategory (): BraveToday.OneOfCategories {
    return 'Entertainment'
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
    const { contentPage } = this.state
    const currentYAxisForPostsBottom = entities[0].boundingClientRect.y

    if (this.state.previousYAxis > currentYAxisForPostsBottom) {
      this.setState({ contentPage: contentPage + 1 })
    }

    this.setState({ previousYAxis: currentYAxisForPostsBottom })
  }

  render () {
    const { feed } = this.props

    if (!feed) {
      // TODO: loading state
      return null
    }

    return (
      <BraveTodayElement.Section>
        <div
          ref={scrollTrigger => (this.scrollTriggerToFocusBraveToday = scrollTrigger)}
          style={{ position: 'sticky', top: '100px' }}
        />
        <CardIntro />
        {/* sponsors */}
        <CardLarge content={[feed.featuredSponsor]} />
        {/* featured item */}
        <CardLarge content={[feed.featuredArticle]} />
        {/* deals */}
        <CardDeals content={feed.featuredDeals} />
        {
          /* Infinitely repeating collections of content. */
          Array.from({ length: this.state.contentPage }).map((_: {}, index: number) => {
            return (
              <>
                <CardsGroup
                  key={`cards-group-key-${index}`}
                  content={feed.scrollingList}
                  // The set of items from a single publisher to be featured in a list
                  featuredPublisher={this.featuredPublisher}
                  // The categorized items that are shown in each interaction
                  featuredCategory={this.featuredCategory}
                  // See https://docs.google.com/document/d/1guIG4Dw0l6REaOclcDjX7acxsDjk_oJlMvLyhD1KDxY/
                  parentIndex={index}
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
