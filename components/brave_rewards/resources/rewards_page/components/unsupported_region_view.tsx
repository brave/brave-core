/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { NewTabLink } from '../../shared/components/new_tab_link'
import * as urls from '../../shared/lib/rewards_urls'

import { style } from './unsupported_region_view.style'

export function UnsupportedRegionView() {
  const { getString } = useLocaleContext()
  return (
    <div data-css-scope={style.scope}>
      <h4>{getString('unsupportedRegionTitle')}</h4>
      <p>{getString('unsupportedRegionText1')}</p>
      <p className='learn-more'>
        <NewTabLink href={urls.rewardsUnavailableURL}>
          {getString('learnMoreLink')}
        </NewTabLink>
      </p>
      <p>
        {getString('unsupportedRegionText2')}{' '}
        {getString('unsupportedRegionText3')}
      </p>
    </div>
  )
}
