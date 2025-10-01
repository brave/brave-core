/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import { CardView } from '../explore/card_view'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './benefits_card.style'

export function BenefitsCard() {
  const { getString } = useLocaleContext()
  const cards = useAppState((state) => state.cards)

  // Use the server-provided benefits card info if available.
  if (cards) {
    const uiCard = cards.find((card) => card.name === 'benefits-card')
    if (uiCard) {
      return <CardView card={uiCard} />
    }
  }

  return (
    <div
      className='content-card'
      data-css-scope={style.scope}
    >
      <h4>{getString('benefitsTitle')}</h4>
      <section>
        <NewTabLink
          className='store'
          href={urls.braveStoreURL}
        >
          <span className='icon'>
            <Icon name='brave-icon-outline' />
          </span>
          <span className='text'>
            <span className='maintext'>
              {getString('benefitsStoreText')}
              <Label color='primary'>{getString('newBadgeText')}</Label>
            </span>
            <span className='subtext'>{getString('benefitsStoreSubtext')}</span>
          </span>
          <Icon name='launch' />
        </NewTabLink>
      </section>
    </div>
  )
}
