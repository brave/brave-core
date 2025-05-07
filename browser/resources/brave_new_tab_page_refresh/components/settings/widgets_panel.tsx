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
  const rewardsFeatureEnabled = useAppState((s) => s.rewardsFeatureEnabled)
  const showRewardsWidget = useAppState((s) => s.showRewardsWidget)
  const vpnFeatureEnabled = useAppState((s) => s.vpnFeatureEnabled)
  const showVpnWidget = useAppState((s) => s.showVpnWidget)

  return (
    <div data-css-scope={style.scope}>
      <div className='control-row'>
        <label>{getString('showStatsLabel')}</label>
        <Toggle
          size='small'
          checked={showStats}
          onChange={({ checked }) => {
            actions.setShowShieldsStats(checked)
          }}
        />
      </div>
      {
        vpnFeatureEnabled &&
          <div className='control-row'>
            <label>{getString('showVpnWidgetLabel')}</label>
            <Toggle
              size='small'
              checked={showVpnWidget}
              onChange={({ checked }) => {
                actions.setShowVpnWidget(checked)
              }}
            />
          </div>
      }
      {
        rewardsFeatureEnabled &&
          <div className='control-row'>
            <label>{getString('showRewardsWidgetLabel')}</label>
            <Toggle
              size='small'
              checked={showRewardsWidget}
              onChange={({ checked }) => {
                actions.setShowRewardsWidget(checked)
              }}
            />
          </div>
      }
      <div className='control-row'>
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
