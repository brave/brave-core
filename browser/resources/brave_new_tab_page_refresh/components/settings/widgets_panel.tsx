/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ControlItem from '@brave/leo/react/controlItem'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import Toggle from '@brave/leo/react/toggle'

import { useLocale } from '../context/locale_context'
import { useNewTabModel, useNewTabState } from '../context/new_tab_context'
import { useVPNModel, useVPNState } from '../context/vpn_context'
import { useRewardsModel, useRewardsState } from '../context/rewards_context'
import { WidgetPosition } from '../../models/new_tab_model'
import { WidgetPositionIcon } from './widget_position_icon'

import { style } from './widgets_panel.style'

export function WidgetsPanel() {
  const { getString } = useLocale()

  const newTabModel = useNewTabModel()
  const vpnModel = useVPNModel()
  const rewardsModel = useRewardsModel()

  const [
    widgetPosition,
    showStats,
    showTalkWidget
  ] = useNewTabState((state) => [
    state.widgetPosition,
    state.showShieldsStats,
    state.showTalkWidget
  ])

  const [
    showVpnWidget,
    vpnFeatureEnabled
  ] = useVPNState((state) => [
    state.showVpnWidget,
    state.vpnFeatureEnabled
  ])

  const [
    showRewardsWidget,
    rewardsFeatureEnabled
  ] = useRewardsState((state) => [
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
                newTabModel.setWidgetPosition(value)
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
            newTabModel.setShowShieldsStats(checked)
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
              onChange={({ checked }) => { vpnModel.setShowVpnWidget(checked) }}
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
                rewardsModel.setShowRewardsWidget(checked)
              }}
            />
          </div>
      }
      <div className='form-control-row'>
        <label>{getString('showTalkWidgetLabel')}</label>
        <Toggle
          size='small'
          checked={showTalkWidget}
          onChange={({ checked }) => { newTabModel.setShowTalkWidget(checked) }}
        />
      </div>
    </div>
  )
}
