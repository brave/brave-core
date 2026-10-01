/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { formatString } from '$web-common/formatString'
import { Link } from '../common/link'
import { getString } from '../../lib/strings'
import {
  BraveBackground,
  SponsoredImageBackground,
} from '../../state/background_store'
import {
  useCurrentBackground,
  useBackgroundActions,
} from '../../context/background_context'
import { useNewTabState } from '../../context/new_tab_context'
import { NewTabTakeoverDisclosure } from './new_tab_takeover_disclosure'

import { style } from './background_caption.style'

export function BackgroundCaption() {
  const currentBackground = useCurrentBackground()
  const centerNttCtaButtonFeatureEnabled = useNewTabState(
    (s) => s.centerNttCtaButtonFeatureEnabled,
  )

  function renderCaption() {
    switch (currentBackground?.type) {
      case 'brave':
        return <BraveBackgroundCredits background={currentBackground} />
      case 'sponsored-image': {
        // The centered-CTA layout only takes effect when the logo actually
        // renders (see `SponsoredBackgroundLogo` below); otherwise the
        // caption stays anchored at the bottom of the viewport, so opening
        // the tooltip downward would push it off-screen.
        const isCenteredCtaButton =
          centerNttCtaButtonFeatureEnabled &&
          !!currentBackground.logo?.imageUrl
        return (
          <>
            <SponsoredBackgroundLogo background={currentBackground} />
            <NewTabTakeoverDisclosure
              background={currentBackground}
              placement={isCenteredCtaButton ? 'bottom' : 'top'}
            />
          </>
        )
      }
      case 'sponsored-rich-media':
        return (
          <NewTabTakeoverDisclosure
            background={currentBackground}
            placement='top'
          />
        )
      default:
        return null
    }
  }

  return <div data-css-scope={style.scope}>{renderCaption()}</div>
}

interface BraveBackgroundCreditsProps {
  background: BraveBackground
}

function BraveBackgroundCredits(props: BraveBackgroundCreditsProps) {
  const { author, link } = props.background
  if (!author) {
    return null
  }
  return (
    <div data-theme='dark'>
      <Link
        className='photo-credits'
        url={link}
      >
        {formatString(getString(S.NEW_TAB_PHOTO_CREDITS_TEXT), [author])}
      </Link>
    </div>
  )
}

interface SponsoredBackgroundLogoProps {
  background: SponsoredImageBackground
}

function SponsoredBackgroundLogo(props: SponsoredBackgroundLogoProps) {
  const actions = useBackgroundActions()
  const centerNttCtaButtonFeatureEnabled = useNewTabState(
    (s) => s.centerNttCtaButtonFeatureEnabled,
  )
  const { logo } = props.background
  if (!logo || !logo.imageUrl) {
    return null
  }
  return (
    <Link
      url={logo.destinationUrl}
      className={
        centerNttCtaButtonFeatureEnabled
          ? 'sponsored-logo centered-ntt-cta-button'
          : 'sponsored-logo'
      }
      onClick={() => actions.notifySponsoredImageLogoClicked()}
      onContextMenu={(e) => e.preventDefault()}
    >
      <Icon name='launch' />
      <img
        src={logo.imageUrl}
        alt={logo.alt}
      />
    </Link>
  )
}
