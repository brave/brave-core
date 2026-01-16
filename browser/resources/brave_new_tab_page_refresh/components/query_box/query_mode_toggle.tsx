/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { style } from './query_mode_toggle.style'

export type QueryMode = 'search' | 'chat'

interface Props {
  queryMode: QueryMode
  onChange: (mode: QueryMode) => void
}

export function QueryModeToggle(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <button
        onClick={() => props.onChange('search')}
        disabled={props.queryMode === 'search'}
      >
        <Icon
          name='search'
          slot='icon-before'
        />
        <span className='name'>Search</span>
      </button>
      <button
        onClick={() => props.onChange('chat')}
        disabled={props.queryMode === 'chat'}
      >
        <Icon
          name='product-brave-leo'
          slot='icon-before'
        />
        <span className='name'>Chat</span>
      </button>
    </div>
  )
}
