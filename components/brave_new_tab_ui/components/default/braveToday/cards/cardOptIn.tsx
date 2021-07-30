// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'
import * as Card from '../cardIntro'
import BraveTodayLogoUrl from '../braveTodayLogo.svg'
import { CardButton, TertiaryButton } from '../default'

type Props = {
  onOptIn: () => unknown
  onDisable: () => unknown
}

export default function IntroCard (props: Props) {
  const introElementRef = React.useRef(null)
  const descriptionTextParts = React.useMemo(() => {
    return getLocaleWithTag('braveTodayIntroDescription')
  }, [])
  return (
    <Card.Intro ref={introElementRef}>
      <Card.Image src={BraveTodayLogoUrl} />
      <Card.Title>{getLocale('braveTodayIntroTitle')}</Card.Title>
      <Card.Paragraph>
        {descriptionTextParts.beforeTag}
        <a href={'https://brave.com/privacy/browser/'}>
          {descriptionTextParts.duringTag}
        </a>
        {descriptionTextParts.afterTag}
      </Card.Paragraph>
      <CardButton onClick={props.onOptIn} isMainFocus={true}>
        {getLocale('braveTodayOptInActionLabel')}
      </CardButton>
      <TertiaryButton onClick={props.onDisable}>
        {getLocale('braveTodayOptOutActionLabel')}
      </TertiaryButton>
    </Card.Intro>
  )
}
