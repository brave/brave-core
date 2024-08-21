/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

import { useLocaleContext } from '../../lib/locale_strings'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './benefits_card.style'

export function BenefitsCard() {
  const { getString } = useLocaleContext()

  return (
    <div className='content-card' {...style}>
      <h4>{getString('benefitsTitle')}</h4>
      <section>
        <NewTabLink className='store' href={urls.braveStoreURL}>
          <span className='icon'>
            <Icon name='brave-icon-outline' />
          </span>
          <span className='text'>
            <span className='maintext'>
              {getString('benefitsStoreText')}
              <Label color='primary'>{getString('newBadgeText')}</Label>
            </span>
            <span className='subtext'>
              {getString('benefitsStoreSubtext')}
            </span>
          </span>
          <Icon name='launch' />
        </NewTabLink>
      </section>
    </div>
  )
}
