/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ControlItem from '@brave/leo/react/controlItem'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import Toggle from '@brave/leo/react/toggle'

import { useLocale } from '../context/locale_context'
import { useAppActions, useAppState } from '../context/app_model_context'
import { WidgetPosition } from '../../models/new_tab'
import { WidgetPositionIcon } from './widget_position_icon'

import { style } from './widgets_panel.style'

export function WidgetsPanel() {
  const { getString } = useLocale()

  const actions = useAppActions()

  const [
    widgetPosition,
    showStats,
    showTalkWidget,
    showVpnWidget,
    vpnFeatureEnabled,
    showRewardsWidget,
    rewardsFeatureEnabled
  ] = useAppState((state) => [
    state.widgetPosition,
    state.showShieldsStats,
    state.showTalkWidget,
    state.showVpnWidget,
    state.vpnFeatureEnabled,
    state.showRewardsWidget,
    state.rewardsFeatureEnabled
  ])

  function renderWidgetPositionItem(position: WidgetPosition) {
    return (
      <ControlItem value={position}>
        <WidgetPositionIcon position={position} />
      </ControlItem>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='form-control-row'>
        <label>{getString('widgetLayoutLabel')}</label>
        <SegmentedControl
          className='layout-control'
          value={widgetPosition}
          onChange={({ value }) => {
            switch (value) {
              case 'top':
              case 'bottom':
                actions.setWidgetPosition(value)
                break
            }
          }}
        >
          {renderWidgetPositionItem('bottom')}
          {renderWidgetPositionItem('top')}
        </SegmentedControl>
      </div>
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
      {
        vpnFeatureEnabled &&
          <div className='form-control-row'>
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
          <div className='form-control-row'>
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
