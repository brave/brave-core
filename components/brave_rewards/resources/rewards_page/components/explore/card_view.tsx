/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../../lib/locale_strings'
import { UICard } from '../../lib/app_state'
import { CardItemView } from './card_item_view'

import { style } from './card_view.style'

export interface Props {
  card: UICard
}

export function CardView(props: Props) {
  const { getString } = useLocaleContext()
  const { card } = props

  function cardTitle() {
    if (card.title) {
      return card.title
    }
    switch (card.name) {
      case 'community-card': return getString('communityTitle')
      case 'bat-card': return getString('batUtilityTitle')
      case 'merch-store-card': return getString('merchStoreTitle')
      case 'benefits-card': return getString('benefitsTitle')
      default: return ''
    }
  }

  return (
    <div className='content-card' data-css-scope={style.scope}>
      <h4>{cardTitle()}</h4>
      <section>
        {card.items.map((item, i) => <CardItemView key={i} item={item} />)}
      </section>
    </div>
  )
}
