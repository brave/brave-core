/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '@brave/leo/react/button'

import { LocaleContext } from '../../lib/locale_context'
import { formatString } from '$web-common/formatString'
import { GeoPinIcon } from '../icons/geo_pin_icon'
import { NewTabLink } from '../new_tab_link'
import { privacyPolicyURL } from '../../lib/rewards_urls'

import * as style from './select_country_card.style'

interface Props {
  onContinue: () => void
}

export function SelectCountryCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.header>
        <GeoPinIcon />{getString('rewardsSelectCountryHeader')}
      </style.header>
      <style.text>
        {
          formatString(getString('rewardsSelectCountryText'), {
            $1: (content) =>
              <NewTabLink href={privacyPolicyURL}>
                {content}
              </NewTabLink>
          })
        }
      </style.text>
      <style.enable>
        <Button onClick={props.onContinue}>
          {getString('rewardsContinue')}
        </Button>
      </style.enable>
    </style.root>
  )
}
