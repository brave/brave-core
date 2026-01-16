/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useShieldsApi } from '../api/shields_api_context'
import { getString } from './strings'

import { style } from './advanced_settings_toggle.style'

export function AdvancedSettingsToggle() {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const { data: showAdvancedView } = api.useGetAdvancedViewEnabled()

  if (!siteBlockInfo) {
    return null
  }

  const shieldsEnabled = siteBlockInfo.isBraveShieldsEnabled
  const adblockOnlyEnabled = siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled

  if (!shieldsEnabled || adblockOnlyEnabled) {
    return null
  }

  return (
    <div
      data-css-scope={style.scope}
      data-expanded={showAdvancedView}
    >
      <button
        className='view-toggle'
        onClick={() => {
          api.setAdvancedViewEnabled([!showAdvancedView])
        }}
      >
        <Icon name='settings' />
        <span>{getString('BRAVE_SHIELDS_ADVANCED_OPTIONS')}</span>
        <Icon name={showAdvancedView ? 'carat-up' : 'carat-down'} />
      </button>
    </div>
  )
}
