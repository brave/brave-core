/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'
import { getString } from '../../lib/strings'

import { style } from './query_mode_toggle.style'
import classnames from '$web-common/classnames'

export type QueryMode = 'search' | 'chat'

interface Props {
  queryMode: QueryMode
  onChange: (mode: QueryMode) => void
}

export function QueryModeToggle(props: Props) {
  return (
    <SegmentedControl
      data-css-scope={style.scope}
      data-test-id='query-mode-toggle'
      size='small'
      value={props.queryMode}
      onChange={({ value }) => {
        props.onChange(value === 'search' ? 'search' : 'chat')
      }}
    >
      <SegmentedControlItem
        value={'chat'}
        className={classnames('item', props.queryMode === 'chat' && 'selected')}
        data-test-id='query-mode-toggle-chat'
      >
        <Icon
          title={getString(S.NEW_TAB_QUERY_TOGGLE_CHAT_LABEL)}
          slot={props.queryMode === 'chat' ? 'icon-before' : 'default'}
          name='product-brave-leo'
        ></Icon>
        {props.queryMode === 'chat' && (
          <span className='name'>
            {getString(S.NEW_TAB_QUERY_TOGGLE_CHAT_LABEL)}
          </span>
        )}
      </SegmentedControlItem>
      <SegmentedControlItem
        className={classnames(
          'item',
          props.queryMode === 'search' && 'selected',
        )}
        value={'search'}
        data-test-id='query-mode-toggle-search'
      >
        <Icon
          title={getString(S.NEW_TAB_QUERY_TOGGLE_SEARCH_LABEL)}
          slot={props.queryMode === 'search' ? 'icon-before' : 'default'}
          name='search'
        ></Icon>
        {props.queryMode === 'search' && (
          <span className='name'>
            {getString(S.NEW_TAB_QUERY_TOGGLE_SEARCH_LABEL)}
          </span>
        )}
      </SegmentedControlItem>
    </SegmentedControl>
  )
}
