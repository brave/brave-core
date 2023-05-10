/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { useLocaleContext } from '../lib/locale_strings'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { Tooltip } from './tooltip'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './verified_tooltip.style'

export function VerifiedTooltip () {
  const { getString } = useLocaleContext()
  return (
    <Tooltip>
      <style.title>
        <style.checkmark>
          <Icon name='verification-filled-color' />
        </style.checkmark>
        <div>{getString('verifiedTooltipTitle')}</div>
      </style.title>
      <style.text>
        {getString('verifiedTooltipText')}&nbsp;
        <NewTabLink href={urls.tippingLearnMoreURL}>
          {getString('learnMoreLabel')}
        </NewTabLink>
      </style.text>
    </Tooltip>
  )
}
