/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DropDown from '@brave/leo/react/dropdown'
import Toggle from '@brave/leo/react/toggle'
import { getLocale } from '$web-common/locale'
import { loadTimeData } from '$web-common/loadTimeData'

import { useBraveNews } from '../shared/Context'
import OpmlControls from './OpmlControls'
import { SettingsPanel } from './SettingsPanel'

import { style } from './NewsSettings.style'

export default function NewsSettings() {
  const {
    isShowOnNTPPrefEnabled,
    toggleBraveNewsOnNTP,
    openArticlesInNewTab,
    setOpenArticlesInNewTab,
  } = useBraveNews()

  const feedV2Enabled = loadTimeData.getBoolean(
    'featureFlagBraveNewsFeedV2Enabled',
  )
  const controlsDisabled = !isShowOnNTPPrefEnabled

  return (
    <SettingsPanel
      cssScope={style.scope}
      title={getLocale(S.BRAVE_NEWS_SETTINGS_SECTION_TITLE)}
    >
      <Toggle
        className='toggle-row'
        size='small'
        checked={isShowOnNTPPrefEnabled}
        onChange={({ checked }) => {
          toggleBraveNewsOnNTP(checked)
        }}
      >
        <span className='label'>
          {getLocale(S.BRAVE_NEWS_ENABLE_LABEL)}
        </span>
      </Toggle>
      {feedV2Enabled && (
        <div className='control-row'>
          <label>{getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN)}</label>
          <DropDown
            value={openArticlesInNewTab ? 'true' : 'false'}
            positionStrategy='fixed'
            disabled={controlsDisabled}
            onChange={(detail) => {
              setOpenArticlesInNewTab(detail.value === 'true')
            }}
          >
            <span slot='value'>
              {openArticlesInNewTab
                ? getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_NEW_TAB)
                : getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_CURRENT_TAB)}
            </span>
            <leo-option value='true'>
              {getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_NEW_TAB)}
            </leo-option>
            <leo-option value='false'>
              {getLocale(S.BRAVE_NEWS_OPEN_ARTICLES_IN_CURRENT_TAB)}
            </leo-option>
          </DropDown>
        </div>
      )}
      <div className='control-row'>
        <OpmlControls disabled={controlsDisabled} />
      </div>
    </SettingsPanel>
  )
}

