/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'
import Button from '@brave/leo/react/button'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'

import { getString } from '../../lib/strings'
import { useNewTabState, useNewTabActions } from '../../context/new_tab_context'
import {
  useRewardsState,
  useRewardsActions,
} from '../../context/rewards_context'
import { useVpnState, useVpnActions } from '../../context/vpn_context'
import {
  useCustomWidgets,
  addCustomWidget,
  removeCustomWidget,
} from '../../state/custom_widgets_store'
import { customWidgetPrompt } from '../../lib/custom_widget_prompt'

import { style } from './widgets_panel.style'

export function WidgetsPanel() {
  const braveNews = useBraveNews()
  const newTabActions = useNewTabActions()
  const rewardsActions = useRewardsActions()
  const vpnActions = useVpnActions()

  const showStats = useNewTabState((s) => s.showShieldsStats)
  const talkFeatureEnabled = useNewTabState((s) => s.talkFeatureEnabled)
  const showTalkWidget = useNewTabState((s) => s.showTalkWidget)
  const newsFeatureEnabled = useNewTabState((s) => s.newsFeatureEnabled)
  const rewardsFeatureEnabled = useRewardsState((s) => s.rewardsFeatureEnabled)
  const showRewardsWidget = useRewardsState((s) => s.showRewardsWidget)
  const vpnFeatureEnabled = useVpnState((s) => s.vpnFeatureEnabled)
  const showVpnWidget = useVpnState((s) => s.showVpnWidget)
  const customWidgetsEnabled = useNewTabState((s) => s.customWidgetsEnabled)

  return (
    <div data-css-scope={style.scope}>
      <Toggle
        className='toggle-row'
        size='small'
        checked={showStats}
        onChange={({ checked }) => {
          newTabActions.setShowShieldsStats(checked)
        }}
      >
        <span className='label'>{getString(S.NEW_TAB_SHOW_STATS_LABEL)}</span>
      </Toggle>
      {vpnFeatureEnabled && (
        <Toggle
          className='toggle-row'
          size='small'
          checked={showVpnWidget}
          onChange={({ checked }) => {
            vpnActions.setShowVpnWidget(checked)
          }}
        >
          <span className='label'>
            {getString(S.NEW_TAB_SHOW_VPN_WIDGET_LABEL)}
          </span>
        </Toggle>
      )}
      {rewardsFeatureEnabled && (
        <Toggle
          className='toggle-row'
          size='small'
          checked={showRewardsWidget}
          onChange={({ checked }) => {
            rewardsActions.setShowRewardsWidget(checked)
          }}
        >
          <span className='label'>
            {getString(S.NEW_TAB_SHOW_REWARDS_WIDGET_LABEL)}
          </span>
        </Toggle>
      )}
      {talkFeatureEnabled && (
        <Toggle
          className='toggle-row'
          size='small'
          checked={showTalkWidget}
          onChange={({ checked }) => {
            newTabActions.setShowTalkWidget(checked)
          }}
        >
          <span className='label'>
            {getString(S.NEW_TAB_SHOW_TALK_WIDGET_LABEL)}
          </span>
        </Toggle>
      )}
      {newsFeatureEnabled && (
        <Toggle
          className='toggle-row'
          size='small'
          checked={braveNews.isShowOnNTPPrefEnabled}
          onChange={({ checked }) => {
            braveNews.toggleBraveNewsOnNTP(checked)
          }}
        >
          <span className='label'>
            {getString(S.NEW_TAB_SHOW_NEWS_WIDGET_LABEL)}
          </span>
        </Toggle>
      )}
      {customWidgetsEnabled && <CustomWidgetsSettings />}
    </div>
  )
}

function CustomWidgetsSettings() {
  const widgets = useCustomWidgets()
  const [name, setName] = React.useState('')
  const [html, setHtml] = React.useState('')

  function onAdd() {
    if (!html.trim()) {
      return
    }
    addCustomWidget(name, html)
    setName('')
    setHtml('')
  }

  function onCopyPrompt() {
    navigator.clipboard.writeText(customWidgetPrompt)
  }

  return (
    <div className='custom-widgets'>
      <div className='custom-widgets-header'>Custom widgets (AI)</div>
      <div className='custom-widgets-description'>
        Ask Leo to build a widget, then paste the HTML it returns below.
      </div>
      <Button
        kind='outline'
        size='small'
        onClick={onCopyPrompt}
      >
        Copy Leo prompt
      </Button>
      <input
        className='custom-widget-name'
        type='text'
        placeholder='Widget name (optional)'
        value={name}
        onChange={(e) => setName(e.currentTarget.value)}
      />
      <textarea
        className='custom-widget-html'
        placeholder='Paste the widget HTML from Leo here'
        value={html}
        onChange={(e) => setHtml(e.currentTarget.value)}
      />
      <Button
        size='small'
        isDisabled={!html.trim()}
        onClick={onAdd}
      >
        Add widget
      </Button>
      {widgets.length > 0 && (
        <ul className='custom-widget-list'>
          {widgets.map((widget) => (
            <li key={widget.id}>
              <span className='custom-widget-list-name'>{widget.name}</span>
              <Button
                kind='plain-faint'
                size='tiny'
                onClick={() => removeCustomWidget(widget.id)}
              >
                Remove
              </Button>
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}
