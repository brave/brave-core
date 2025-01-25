/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import formatMessage from '$web-common/formatMessage'
import { sanitizeExternalURL } from '../lib/url_sanitizer'
import { useLocale } from './locale_context'
import { useNewTabState } from './new_tab_context'

import {
  BraveBackground,
  SponsoredImageBackground } from '../models/new_tab_model'

import { style } from './background_caption.style'

function BraveBackgroundCredits(
  props: { background: BraveBackground}
) {
  const { getString } = useLocale()
  const { author, link } = props.background
  if (!author) {
    return null
  }
  const text = formatMessage(getString('photoCreditsText'), [author])
  const sanitizedLink = sanitizeExternalURL(link)
  if (!sanitizedLink) {
    return <span className='photo-credits'>{text}</span>
  }
  return (
    <a
      className='photo-credits'
      href={sanitizedLink}
      target='_blank'
      rel='noopener noreferrer'
    >
      {formatMessage(getString('photoCreditsText'), [author])}
    </a>
  )
}

function SponsoredBackgroundLogo(
  props: { background: SponsoredImageBackground }
) {
  if (!props.background.logo) {
    return null
  }
  const { logo } = props.background
  return (
    <a
      className='sponsored-logo'
      href={sanitizeExternalURL(logo.destinationUrl)}
      target='_blank'
      rel='noopener noreferrer'
    >
      <Icon name='launch' />
      <img src={logo.imageUrl} alt={logo.alt} />
    </a>
  )
}

export function BackgroundCaption() {
  const currentBackground = useNewTabState((state) => state.currentBackground)

  function renderCaption() {
    switch (currentBackground?.type) {
      case 'brave':
        return <BraveBackgroundCredits background={currentBackground} />
      case 'sponsored':
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
