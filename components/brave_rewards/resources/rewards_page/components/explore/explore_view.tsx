/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import { useBreakpoint } from '../../lib/breakpoint'
import { UICard } from '../../lib/app_state'
import { CardView } from './card_view'

import { style } from './explore_view.style'

export function ExploreView() {
  const { getString } = useLocaleContext()
  const viewType = useBreakpoint()
  let cards = useAppState((state) => state.cards)

  if (!cards) {
    return (
      <div data-css-scope={style.scope}>
        <div className='loading'>
          <ProgressRing />
        </div>
      </div>
    )
  }

  cards = getExploreCards(cards)

  if (viewType === 'double') {
    const [left, right] = splitCardsIntoColumns(cards)
    return (
      <div data-css-scope={style.scope}>
        <h3>{getString('navigationExploreLabel')}</h3>
        <div className='columns'>
          <div>
            {left.map((card) => <CardView key={card.name} card={card} />)}
          </div>
          <div>
            {right.map((card) => <CardView key={card.name} card={card} />)}
          </div>
        </div>
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <h3>{getString('navigationExploreLabel')}</h3>
      {cards.map((card) => <CardView key={card.name} card={card} />)}
    </div>
  )
}

function getExploreCards(cards: UICard[]) {
  return cards.filter((card) => card.name !== 'benefits-card')
}

function splitCardsIntoColumns(cards: UICard[]) {
  const left: UICard[] = []
  const right: UICard[] = []
  for (const card of cards) {
    if (left.length > right.length) {
      right.push(card)
    } else {
      left.push(card)
    }
  }
  return [left, right]
}
