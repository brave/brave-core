/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState, AppModelContext } from '../../lib/app_model_context'
import { useBreakpoint } from '../../lib/breakpoint'
import { UICard } from '../../lib/app_state'
import { CardView, sortCards, splitCardsIntoColumns } from './card_view'
import { useOnVisibleCallback } from '../../../../../common/useVisible'

import { style } from './explore_view.style'

export function ExploreView() {
  const { getString } = useLocaleContext()
  const viewType = useBreakpoint()
  const model = React.useContext(AppModelContext)
  const ref = React.useRef<HTMLDivElement>(null)
  let cards = useAppState((state) => state.cards)

  // Record offer view when the explore section becomes visible.
  const { setElementRef } = useOnVisibleCallback(() => {
    model.recordOfferView()
  }, {})

  React.useEffect(() => {
    setElementRef(ref.current)
  }, [])

  // If a card ID is present in the hash portion of the URL, scroll to that
  // card.
  React.useEffect(() => {
    const id = location.hash.replace(/^#/, '')
    if (!id || !cards) {
      return
    }
    const elem = ref.current?.querySelector(`[data-deep-link-id=${id}]`)
    if (elem instanceof HTMLElement) {
      elem.offsetParent?.scrollTo({ top: elem.offsetTop - 16 })
    }
  }, [cards])

  if (!cards) {
    return (
      <div data-css-scope={style.scope}>
        <div className='loading'>
          <ProgressRing />
        </div>
      </div>
    )
  }

  cards = sortCards(getExploreCards(cards))

  if (viewType === 'double') {
    const [left, right] = splitCardsIntoColumns(cards)
    return (
      <div
        ref={ref}
        data-css-scope={style.scope}
      >
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
    <div ref={ref} data-css-scope={style.scope}>
      <h3>{getString('navigationExploreLabel')}</h3>
      {cards.map((card) => <CardView key={card.name} card={card} />)}
    </div>
  )
}

function getExploreCards(cards: UICard[]) {
  return cards.filter((card) => {
    if (!card.section) {
      return card.name !== 'benefits-card'
    }
    return card.section === 'explore'
  })
}
