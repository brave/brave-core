/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../../lib/locale_strings'
import { UICard } from '../../lib/app_state'
import { AppModelContext } from '../../lib/app_model_context'
import { CardItemView } from './card_item_view'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { sanitizeURL, cardImageURL } from './card_urls'

import { style } from './card_view.style'

export interface Props {
  card: UICard
}

export function CardView(props: Props) {
  const { getString } = useLocaleContext()
  const { card } = props
  const model = React.useContext(AppModelContext)

  function cardTitle() {
    if (card.title) {
      return card.title
    }
    switch (card.name) {
      case 'community-card':
        return getString('communityTitle')
      case 'bat-card':
        return getString('batUtilityTitle')
      case 'merch-store-card':
        return getString('merchStoreTitle')
      case 'benefits-card':
        return getString('benefitsTitle')
      default:
        return ''
    }
  }

  return (
    <div
      className='content-card'
      data-css-scope={style.scope}
      data-deep-link-id={card.name}
    >
      <h4>{cardTitle()}</h4>
      {card.banner && (
        <NewTabLink
          className='banner'
          href={sanitizeURL(card.banner.url)}
          onClick={() => model.recordOfferClick()}
        >
          <img src={cardImageURL(card.banner.image)} />
        </NewTabLink>
      )}
      <section>
        {card.items.map((item, i) => (
          <CardItemView
            key={i}
            item={item}
          />
        ))}
      </section>
    </div>
  )
}

export function sortCards(cards: UICard[]) {
  return cards.sort((a, b) => a.order - b.order)
}

export function splitCardsIntoColumns(cards: UICard[]) {
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
