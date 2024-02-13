// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'
import * as Card from '../cardIntro'
import BraveNewsLogoUrl from '../braveNewsLogo.svg'
import { CardButton, TertiaryButton } from '../default'
import { NEWS_FEED_CLASS } from '../../../../../brave_news/browser/resources/Feed'

type Props = {
  onOptIn: () => unknown
  onDisable: () => unknown
}

const descriptionTwoTextParts = getLocaleWithTag('braveNewsIntroDescriptionTwo')

export default function IntroCard (props: Props) {
  const introElementRef = React.useRef(null)
  return (
    <Card.Intro ref={introElementRef} className={NEWS_FEED_CLASS}>
      <Card.Image src={BraveNewsLogoUrl} />
      <Card.Title>{getLocale('braveNewsIntroTitle')}</Card.Title>
      <div>
        <Card.Paragraph>
          {getLocale('braveNewsIntroDescription')}
        </Card.Paragraph>
        <Card.Paragraph>
          {descriptionTwoTextParts.beforeTag}
          <a href={'https://brave.com/privacy/browser/'}>
            {descriptionTwoTextParts.duringTag}
          </a>
          {descriptionTwoTextParts.afterTag}
        </Card.Paragraph>
      </div>
      <CardButton onClick={props.onOptIn} isMainFocus={true}>
        {getLocale('braveNewsOptInActionLabel')}
      </CardButton>
      <TertiaryButton onClick={props.onDisable}>
        {getLocale('braveNewsOptOutActionLabel')}
      </TertiaryButton>
    </Card.Intro>
  )
}
