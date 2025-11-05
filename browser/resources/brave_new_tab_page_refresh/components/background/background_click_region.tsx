/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { Link } from '../common/link'
import { useCurrentBackground, useBackgroundActions } from '../../context/background_context'

import { style } from './background_click_region.style'

export function BackgroundClickRegion() {
  const actions = useBackgroundActions()
  const currentBackground = useCurrentBackground()
  if (currentBackground?.type !== 'sponsored-image') {
    return null
  }

  const url = currentBackground.logo?.destinationUrl
  if (!url) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <Link url={url} onClick={actions.notifySponsoredImageLogoClicked}>
        <Icon name='launch' />
      </Link>
    </div>
  )
}
