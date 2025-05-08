/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import { useLocale } from '../context/locale_context'
import { useNewTabState, useNewTabActions } from '../context/new_tab_context'
import { useRewardsState, useRewardsActions } from '../context/rewards_context'
import { useVpnState, useVpnActions } from '../context/vpn_context'
import { useNewsState, useNewsActions } from '../context/news_context'

import { style } from './widgets_panel.style'

export function WidgetsPanel() {
  const { getString } = useLocale()

  const newTabActions = useNewTabActions()
  const rewardsActions = useRewardsActions()
  const vpnActions = useVpnActions()
  const newsActions = useNewsActions()

  const showStats = useNewTabState((s) => s.showShieldsStats)
  const showTalkWidget = useNewTabState((s) => s.showTalkWidget)
  const rewardsFeatureEnabled = useRewardsState((s) => s.rewardsFeatureEnabled)
  const showRewardsWidget = useRewardsState((s) => s.showRewardsWidget)
  const vpnFeatureEnabled = useVpnState((s) => s.vpnFeatureEnabled)
  const showVpnWidget = useVpnState((s) => s.showVpnWidget)
  const showNewsWidget = useNewsState((s) => s.showNewsWidget)

  return (
    <div data-css-scope={style.scope}>
      <div className='form-control-row'>
        <label>{getString('showStatsLabel')}</label>
        <Toggle
          size='small'
          checked={showStats}
          onChange={({ checked }) => {
            newTabActions.setShowShieldsStats(checked)
          }}
        />
      </div>
      <div className='form-control-row'>
        <label>{getString('showNewsWidgetLabel')}</label>
        <Toggle
          size='small'
          checked={showNewsWidget}
          onChange={({ checked }) => {
            newsActions.setShowNewsWidget(checked)
          }}
        />
      </div>
      {
        vpnFeatureEnabled &&
          <div className='form-control-row'>
            <label>{getString('showVpnWidgetLabel')}</label>
            <Toggle
              size='small'
              checked={showVpnWidget}
              onChange={({ checked }) => {
                vpnActions.setShowVpnWidget(checked)
              }}
            />
          </div>
      }
      {
        rewardsFeatureEnabled &&
          <div className='form-control-row'>
            <label>{getString('showRewardsWidgetLabel')}</label>
            <Toggle
              size='small'
              checked={showRewardsWidget}
              onChange={({ checked }) => {
                rewardsActions.setShowRewardsWidget(checked)
              }}
            />
          </div>
      }
      <div className='form-control-row'>
        <label>{getString('showTalkWidgetLabel')}</label>
        <Toggle
          size='small'
          checked={showTalkWidget}
          onChange={({ checked }) => {
            newTabActions.setShowTalkWidget(checked)
          }}
        />
      </div>
    </div>
  )
}
