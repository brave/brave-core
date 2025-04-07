/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../context/app_model_context'
import { NtpWidget } from './ntp_widget'
import { RewardsWidget } from './rewards_widget'
import { TalkWidget } from './talk_widget'
import { VPNWidget } from './vpn_widget'
import { StatsWidget } from './stats_widget'
import { NewsWidget } from './news_widget'

import { style } from './widget_stack.style'

type TabName = 'rewards' | 'talk' | 'vpn' | 'stats' | 'news'

interface Props {
  name: string
  tabs: TabName[]
}

export function WidgetStack(props: Props) {
  const showTalkWidget = useAppState((s) => s.showTalkWidget)
  const showRewardsWidget = useAppState((s) => s.showRewardsWidget)
  const rewardsFeatureEnabled = useAppState((s) => s.rewardsFeatureEnabled)
  const vpnFeatureEnabled = useAppState((s) => s.vpnFeatureEnabled)
  const showVpnWidget = useAppState((s) => s.showVpnWidget)
  const showShieldsStats = useAppState((s) => s.showShieldsStats)
  const showNewsWidget = useAppState((s) => s.showNewsWidget)

  const [currentTab, setCurrentTab] = React.useState(loadCurrentTab(props.name))

  const visibleTabs = React.useMemo(() => {
    return props.tabs.filter((tab) => {
      switch (tab) {
        case 'rewards': return rewardsFeatureEnabled && showRewardsWidget
        case 'talk': return showTalkWidget
        case 'vpn': return vpnFeatureEnabled && showVpnWidget
        case 'stats': return showShieldsStats
        case 'news': return showNewsWidget
      }
    })
  }, [
    props.tabs,
    showTalkWidget,
    rewardsFeatureEnabled,
    showRewardsWidget,
    vpnFeatureEnabled,
    showVpnWidget,
    showShieldsStats,
    showNewsWidget
  ])

  React.useEffect(() => {
    storeCurrentTab(props.name, currentTab)
    if (currentTab && !visibleTabs.includes(currentTab)) {
      setCurrentTab(null)
    }
  }, [currentTab, visibleTabs])

  function renderTabButton(tab: TabName) {
    return (
      <button
        key={tab}
        className={tab === activeTab ? 'active' : ''}
        onClick={() => setCurrentTab(tab)}
      >
        {renderProductIcon(tab)}
      </button>
    )
  }

  function renderProductIcon(tab: TabName) {
    switch (tab) {
      case 'rewards': return <Icon name='product-bat-outline' />
      case 'talk': return <Icon name='product-brave-talk' />
      case 'vpn': return <Icon name='product-vpn' />
      case 'stats': return <Icon name='bar-chart' />
      case 'news': return <Icon name='product-brave-news' />
    }
  }

  function renderWidget() {
    switch (activeTab) {
      case 'rewards': return <RewardsWidget />
      case 'talk': return <TalkWidget />
      case 'vpn': return <VPNWidget />
      case 'stats': return <StatsWidget />
      case 'news': return <NewsWidget />
    }
  }

  if (visibleTabs.length === 0) {
    return null
  }

  const activeTab = currentTab || visibleTabs[0]

  return (
    <NtpWidget>
      <div data-css-scope={style.scope}>
        {
          visibleTabs.length > 1 &&
            <div className='stack-tabs'>
              {visibleTabs.map(renderTabButton)}
            </div>
        }
        <div className='widget'>
          {renderWidget()}
        </div>
      </div>
    </NtpWidget>
  )
}

function loadCurrentTab(stackName: string): TabName | null {
  const value = localStorage.getItem(storageKey(stackName))
  return tabNameIdentity(value as TabName) ?? null
}

function tabNameIdentity(tabName: TabName): TabName {
  switch (tabName) {
    case 'vpn':
    case 'rewards':
    case 'talk':
    case 'stats':
    case 'news':
      return tabName
  }
}

function storeCurrentTab(stackName: string, tab: TabName | null) {
  localStorage.setItem(storageKey(stackName), tab ?? '')
}

function storageKey(stackName: string) {
  return `ntp-current-${stackName}-widget`
}
