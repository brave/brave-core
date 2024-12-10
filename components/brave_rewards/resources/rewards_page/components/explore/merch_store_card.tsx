/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { CardItemView } from './card_item_view'

import * as urls from '../../../shared/lib/rewards_urls'

export function MerchStoreCard() {
  const { getString } = useLocaleContext()
  const [cards] = useAppState((state) => [state.cards])

  if (!cards) {
    return null
  }

  const card = cards.find((value) => value.name === 'merch-store-card')
  if (!card) {
    return null
  }

  return (
    <div className='content-card'>
      <h4>
        {getString('merchStoreTitle')}
        <NewTabLink href={urls.braveStoreURL}>
          {getString('viewStoreLink')}
          <Icon name='launch' />
        </NewTabLink>
      </h4>
      <section>
        {card.items.map((item, i) => <CardItemView key={i} item={item} />)}
      </section>
    </div>
  )
}
