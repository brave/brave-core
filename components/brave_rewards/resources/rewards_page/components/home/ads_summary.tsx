/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Label from '@brave/leo/react/label'

import { AdType } from '../../lib/app_model'
import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'

import { style } from './ads_summary.style'

export function AdsSummary() {
  const { getString } = useLocaleContext()
  const [adsInfo] = useAppState((state) => [state.adsInfo])

  function renderRow(text: string, adType: AdType) {
    if (!adsInfo) {
      return null
    }
    const enabled = adsInfo.adsEnabled[adType]
    return (
      <div>
        <span>{text}</span>
        <span className='value'>
          {adsInfo.adTypesReceivedThisMonth[adType]}
        </span>
        <Label color={enabled ? 'green' : 'red'}>
          <span className='toggle-text'>
            {enabled ? getString('adTypeOnLabel') : getString('adTypeOffLabel')}
          </span>
        </Label>
      </div>
    )
  }

  return (
    <div {...style}>
      {renderRow(getString('adTypeNewTabPageLabel'), 'new-tab-page')}
      {renderRow(getString('adTypeNotificationLabel'), 'notification')}
      {renderRow(getString('adTypeSearchResultLabel'), 'search-result')}
      {renderRow(getString('adTypeInlineContentLabel'), 'inline-content')}
    </div>
  )
}
