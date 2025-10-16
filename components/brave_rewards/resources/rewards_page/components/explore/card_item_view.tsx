/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { UICardItem } from '../../lib/app_state'
import { AppModelContext } from '../../lib/app_model_context'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { sanitizeURL, faviconURL, cardImageURL } from './card_urls'

interface Props {
  item: UICardItem
}

export function CardItemView(props: Props) {
  const { item } = props
  const model = React.useContext(AppModelContext)
  const thumbnail = cardImageURL(item.thumbnail)
  return (
    <NewTabLink
      href={sanitizeURL(item.url)}
      onClick={() => model.recordOfferClick()}
    >
      <span className='thumbnail'>
        {thumbnail ? (
          <img src={thumbnail} />
        ) : (
          <img
            className='favicon'
            src={faviconURL(item.url)}
          />
        )}
      </span>
      <span className='item-info'>
        <span className='title'>{item.title}</span>
        <span className='description'>{item.description}</span>
      </span>
    </NewTabLink>
  )
}
