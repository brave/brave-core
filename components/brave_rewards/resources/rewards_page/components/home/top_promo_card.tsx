/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState } from '../../lib/app_model_context'
import { CardView } from '../explore/card_view'

export function TopPromoCard() {
  const cards = useAppState((state) => state.cards)
  const isBubble = useAppState((state) => state.embedder.isBubble)

  if (!cards || !isBubble) {
    return null
  }

  const promoCards = cards.filter((card) => card.section === 'top')
  if (promoCards.length === 0) {
    return null
  }

  return promoCards.map((card) => (
    <CardView
      key={card.name}
      card={card}
    />
  ))
}
