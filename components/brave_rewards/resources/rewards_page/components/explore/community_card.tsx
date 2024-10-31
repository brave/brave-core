/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import { UICardItem } from '../../lib/app_state'
import { NewTabLink } from '../../../shared/components/new_tab_link'

import * as urls from '../../../shared/lib/rewards_urls'

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

  function renderItem(item: UICardItem) {
    return (
      <NewTabLink key={item.url} href={item.url}>
        <img src={item.thumbnail} />
        <span className='item-info'>
          <span className='title'>{item.title}</span>
          <span className='description'>{item.description}</span>
        </span>
      </NewTabLink>
    )
  }

  return (
    <div className='content-card'>
      <h4>
        {getString('communityTitle')}
        <NewTabLink href={urls.contactSupportURL}>
        {getString('viewAllLink')}
        </NewTabLink>
      </h4>
      <section>
        {card.items.map(renderItem)}
      </section>
    </div>
  )
}
