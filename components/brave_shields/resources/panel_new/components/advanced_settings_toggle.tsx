/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useShieldsState, useShieldsActions } from '../lib/shields_context'
import { getString } from '../lib/strings'

import { style } from './advanced_settings_toggle.style'

export function AdvancedSettingsToggle() {
  const actions = useShieldsActions()
  const shieldsEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsEnabled,
  )
  const adblockOnlyEnabled = useShieldsState(
    (s) => s.siteBlockInfo.isBraveShieldsAdBlockOnlyModeEnabled,
  )
  const showAdvancedSettings = useShieldsState((s) => s.showAdvancedSettings)

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
