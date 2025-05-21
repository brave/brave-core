/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Toggle from '@brave/leo/react/toggle'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'

import { getString } from '../../lib/strings'
import { dispatchCustomEvent } from '../../lib/custom_event'

import { style } from './news_panel.style'

export function NewsPanel() {
  const braveNews = useBraveNews()
  const isOptedIn = braveNews.isOptInPrefEnabled
  const showOnNTP = braveNews.isShowOnNTPPrefEnabled

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString('showNewsLabel')}</label>
        <Toggle
          size='small'
          checked={showOnNTP}
          onChange={({ checked }) => braveNews.toggleBraveNewsOnNTP(checked)}
        />
      </div>
      {
        isOptedIn && showOnNTP &&
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
