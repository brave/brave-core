/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState } from '../../lib/app_model_context'
import { CardItemView } from './card_item_view'

export function PartnerPromoCard() {
  const cards = useAppState((state) => state.cards)

  if (!cards) {
    return null
  }

  const card = cards.find((value) => value.name === 'partner-promo-card')
  if (!card || !card.title || card.items.length === 0) {
    return null
  }

  return (
    <div className='content-card'>
      <h4>{card.title}</h4>
      <section>
        {card.items.map((item, i) => <CardItemView key={i} item={item} />)}
      </section>
    </div>
  )
}
