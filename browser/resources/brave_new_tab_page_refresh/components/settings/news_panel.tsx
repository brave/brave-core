/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Toggle from '@brave/leo/react/toggle'

import { useLocale } from '../../context/locale_context'
import { useNewsState, useNewsActions } from '../../context/news_context'
import { dispatchCustomEvent } from '../../lib/custom_event'

import { style } from './news_panel.style'

export function NewsPanel() {
  const { getString } = useLocale()
  const actions = useNewsActions()
  const showNewsFeed = useNewsState((s) => s.showNewsFeed)
  const newsEnabled = useNewsState((s) => s.newsEnabled)

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString('showNewsLabel')}</label>
        <Toggle
          size='small'
          checked={showNewsFeed}
          onChange={({ checked }) => {
            actions.setShowNewsFeed(checked)
            actions.setNewsEnabled(checked)
          }}
        />
      </div>
      {
        newsEnabled && showNewsFeed &&
          <div className='actions'>
            <Button
              size='small'
              onClick={() => dispatchCustomEvent('ntp-open-news-feed-settings')}
            >
              {getString('newsManageFeedsButtonLabel')}
            </Button>
          </div>
      }
    </div>
  )
}
