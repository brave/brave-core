// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import VisibilityTimer from '../../../../../helpers/visibilityTimer'
import { getLocale } from '../../../../../../common/locale'
import * as Card from '../../cardSizes'
import useScrollIntoView from '../../useScrollIntoView'
import { useVisitDisplayAdClickHandler } from '../../useReadArticleClickHandler'
import { OnVisitDisplayAd, OnViewedDisplayAd, GetDisplayAdContent } from '../..'
import CardImage from '../CardImage'
import * as Styles from './style'

type Props = {
  shouldScrollIntoView?: boolean
  getContent: GetDisplayAdContent
  onVisitDisplayAd: OnVisitDisplayAd
  onViewedDisplayAd: OnViewedDisplayAd
}

export default function CardDisplayAd (props: Props) {
  // Content is retrieved when the element is close to the viewport
  //  - undefined = not fetched yet
  //  - null = no ad available for this unit
  //  - DisplayAd = ad for this unit
  const [content, setContent] = React.useState<BraveToday.DisplayAd | undefined | null>(undefined)
  const [cardRef] = useScrollIntoView(props.shouldScrollIntoView || false)
  const onClick = useVisitDisplayAdClickHandler(props.onVisitDisplayAd, content ? { ad: content } : undefined)
  const innerRef = React.useRef<HTMLElement>(null)

  React.useEffect(() => {
    if (!innerRef.current || !content || !props.onViewedDisplayAd) {
      return
    }
    // Detect when card is viewed, and send an action.
    let onItemViewed = props.onViewedDisplayAd
    const observer = new VisibilityTimer(() => {
      onItemViewed({ ad: content })
    }, 100, innerRef.current)
    observer.startTracking()
    return () => {
      observer.stopTracking()
    }
  }, [innerRef.current, props.onViewedDisplayAd, content?.uuid])
  // Ask for and render the ad only when we're scrolled close to it
  const contentTrigger = React.useRef<HTMLDivElement>(null)
  const contentTriggerObserver = React.useRef<IntersectionObserver>()
  React.useEffect(() => {
    // Setup observer on first mount
    contentTriggerObserver.current = new IntersectionObserver(async (entries) => {
      if (entries.some((entry) => entry.isIntersecting)) {
        // Get the ad and display it
        const ad = await props.getContent()
        // Request may not actually come back with an ad
        if (ad) {
          setContent(ad)
        }
      }
    }, {
      // Trigger ad fetch when the ad unit is 1000px away from the viewport
      rootMargin: '0px 0px 1000px 0px'
    })
  })
  React.useEffect(() => {
    // Observe content trigger when it's appropriate.
    // Don't observe (or disconnect current observer) if there is already content
    if (content || !contentTrigger.current || !contentTriggerObserver.current) {
      return
    }
    const observer = contentTriggerObserver.current
    observer.observe(contentTrigger.current)
    return () => {
      observer.disconnect()
    }
  }, [content, contentTrigger.current, contentTriggerObserver.current])
  // Render content trigger
  if (!content) {
    // verbose ref type conversion due to https://stackoverflow.com/questions/61102101/cannot-assign-refobjecthtmldivelement-to-refobjecthtmlelement-instance
    return <div ref={contentTrigger}><div ref={cardRef as unknown as React.RefObject<HTMLDivElement>} /></div>
  }
  // Render ad when one is available for this unit
  // TODO(petemill): Avoid nested links
  return (
    <Card.Large ref={innerRef}>
      <Styles.BatAdLabel href='chrome://rewards'>
        {getLocale('ad')}
      </Styles.BatAdLabel>
      <a onClick={onClick} href={content.targetUrl} ref={cardRef}>
        <CardImage
          imageUrl={content.imageUrl}
          isPromoted={true}
        />
        <Card.Content>
          <Styles.Header>
            <Card.Heading>
              {content.title}
            </Card.Heading>
            <Styles.CallToAction onClick={onClick}>
              {content.ctaText}
            </Styles.CallToAction>
          </Styles.Header>
          {
            <Card.Source>
              <Card.Publisher>
                {content.description}
              </Card.Publisher>
            </Card.Source>
          }
        </Card.Content>
      </a>
    </Card.Large>
  )
}
