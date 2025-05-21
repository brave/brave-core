/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Toggle from '@brave/leo/react/toggle'

import { getString } from '../../lib/strings'
import { useNewsState, useNewsActions } from '../../context/news_context'
import { dispatchCustomEvent } from '../../lib/custom_event'

import { style } from './news_panel.style'

export function NewsPanel() {
  const actions = useNewsActions()
  const showOnNTP = useNewsState((s) => s.showOnNTP)
  const isOptedIn = useNewsState((s) => s.isOptedIn)

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString(S.NEW_TAB_SHOW_NEWS_LABEL)}</label>
        <Toggle
          size='small'
          checked={showOnNTP}
          onChange={({ checked }) => {
            actions.setShowOnNTP(checked)
            actions.setIsOptedIn(checked)
          }}
        />
      </div>
      {isOptedIn && showOnNTP && (
        <div className='actions'>
          <Button
            size='small'
            onClick={() => dispatchCustomEvent('ntp-open-news-feed-settings')}
          >
            {getString(S.NEW_TAB_NEWS_MANAGE_FEEDS_BUTTON_LABEL)}
          </Button>
        </div>
      )}
    </div>
  )
}
