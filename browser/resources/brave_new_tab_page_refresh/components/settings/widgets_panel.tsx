/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import { useLocale } from '../context/locale_context'
import { useAppActions, useAppState } from '../context/app_model_context'

import { style } from './widgets_panel.style'

export function WidgetsPanel() {
  const { getString } = useLocale()

  const actions = useAppActions()

  const showStats = useAppState((s) => s.showShieldsStats)
  const showTalkWidget = useAppState((s) => s.showTalkWidget)

  return (
    <div data-css-scope={style.scope}>
      <div className='form-control-row'>
        <label>{getString('showStatsLabel')}</label>
        <Toggle
          size='small'
          checked={showStats}
          onChange={({ checked }) => {
            actions.setShowShieldsStats(checked)
          }}
        />
      </div>
      <div className='form-control-row'>
        <label>{getString('showTalkWidgetLabel')}</label>
        <Toggle
          size='small'
          checked={showTalkWidget}
          onChange={({ checked }) => { actions.setShowTalkWidget(checked) }}
        />
      </div>
    </div>
  )
}
