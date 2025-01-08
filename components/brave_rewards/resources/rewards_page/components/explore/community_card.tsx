/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import { CardItemView } from './card_item_view'

export function CommunityCard() {
  const { getString } = useLocaleContext()
  const [cards] = useAppState((state) => [state.cards])

  if (!cards) {
    return null
  }

  const card = cards.find((value) => value.name === 'community-card')
  if (!card) {
    return null
  }

  return (
    <div className='content-card'>
      <h4>
        {getString('communityTitle')}
      </h4>
      <section>
        {card.items.map((item, i) => <CardItemView key={i} item={item} />)}
      </section>
    </div>
  )
}
