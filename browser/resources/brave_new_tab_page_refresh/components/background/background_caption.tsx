/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import formatMessage from '$web-common/formatMessage'
import { Link } from '../common/link'
import { useLocale } from '../context/locale_context'
import { useAppActions, useAppState } from '../context/app_model_context'
import { BraveBackground, SponsoredImageBackground } from '../../models/backgrounds'

import { style } from './background_caption.style'

export function BackgroundCaption() {
  const currentBackground = useAppState((state) => state.currentBackground)

  function renderCaption() {
    switch (currentBackground?.type) {
      case 'brave':
        return <BraveBackgroundCredits background={currentBackground} />
      case 'sponsored-image':
        return <SponsoredBackgroundLogo background={currentBackground} />
      default:
        return null
    }
  }

  return (
    <div data-css-scope={style.scope}>
      {renderCaption()}
    </div>
  )
}

function BraveBackgroundCredits(props: { background: BraveBackground}) {
  const { getString } = useLocale()
  const { author, link } = props.background
  if (!author) {
    return null
  }
  return (
    <Link className='photo-credits' url={link}>
      {formatMessage(getString('photoCreditsText'), [author])}
    </Link>
  )
}

function SponsoredBackgroundLogo(
  props: { background: SponsoredImageBackground }
) {
  const actions = useAppActions()
  if (!props.background.logo) {
    return null
  }
  const { logo } = props.background
  return (
    <Link
      url={logo.destinationUrl}
      className='sponsored-logo'
      onClick={() => actions.notifySponsoredImageLogoClicked()}
    >
      <Icon name='launch' />
      <img src={logo.imageUrl} alt={logo.alt} />
    </Link>
  )
}
