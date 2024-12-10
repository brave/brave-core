/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { UICardItem } from '../../lib/app_state'
import { NewTabLink } from '../../../shared/components/new_tab_link'

function sanitizeURL(url: string) {
  try {
    return new URL(url).protocol === 'https:' ? url : ''
  } catch {
    return ''
  }
}

function thumbnailURL(url: string) {
  url = sanitizeURL(url)
  try {
    if (/(^|\.)brave\.com$/i.test(new URL(url).hostname)) {
      return `chrome://rewards-image/${url}`
    }
    return ''
  } catch {
    return ''
  }
}

interface Props {
  item: UICardItem
}

export function CardItemView(props: Props) {
  const { item } = props
  return (
    <NewTabLink href={sanitizeURL(item.url)}>
      <img src={thumbnailURL(item.thumbnail)} />
      <span className='item-info'>
        <span className='title'>{item.title}</span>
        <span className='description'>{item.description}</span>
      </span>
    </NewTabLink>
  )
}
