// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../common/locale'
import VisibilityTimer from '../../../../helpers/visibilityTimer'
import * as Card from '../cardIntro'
import BraveTodayLogoUrl from '../braveTodayLogo.svg'

const timeToHideMs = 4000

type Props = {
  onRead: () => void
}

export default function IntroCard (props: Props) {
  const introElementRef = React.useRef(null)

  // Only mark as 'read' when it's been in the viewport for a
  // specific amount of time, and the tab is active.
  React.useEffect(() => {
    const element = introElementRef.current
    if (!element) {
      return
    }
    const observer = new VisibilityTimer(props.onRead, timeToHideMs, element)
    observer.startTracking()
    return () => {
      observer.stopTracking()
    }
  }, [introElementRef.current, props.onRead])

  return (
    <Card.Intro innerRef={introElementRef}>
      <Card.Image src={BraveTodayLogoUrl} />
      <Card.Heading>{getLocale('braveTodayIntroTitle')}</Card.Heading>
      <Card.Text>{getLocale('braveTodayIntroDescription')}</Card.Text>
    </Card.Intro>
  )
}
