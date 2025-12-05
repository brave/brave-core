/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useAppState, useAppActions } from './app_context'
import { getString } from '../lib/strings'

import { style } from './advanced_settings_toggle.style'

export function AdvancedSettingsToggle() {
  const actions = useAppActions()
  const shieldsEnabled = useAppState(
    (s) => s.siteBlockInfo.isBraveShieldsEnabled,
  )
  const adblockOnlyEnabled = useAppState(
    (s) => s.siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled,
  )
  const showAdvancedSettings = useAppState((s) => s.showAdvancedSettings)

  if (!shieldsEnabled || adblockOnlyEnabled) {
    return null
  }

  return (
    <div
      data-css-scope={style.scope}
      data-expanded={showAdvancedSettings}
    >
      <button
        className='view-toggle'
        onClick={() => {
          actions.setShowAdvancedSettings(!showAdvancedSettings)
        }}
      >
        <Icon name='settings' />
        <span>{getString('BRAVE_SHIELDS_ADVANCED_OPTIONS')}</span>
        <Icon name={showAdvancedSettings ? 'carat-up' : 'carat-down'} />
      </button>
    </div>
  )
}
