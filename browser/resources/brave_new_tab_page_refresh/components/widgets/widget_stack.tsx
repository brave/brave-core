/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'
import { useNewTabState } from '../../context/new_tab_context'
import { useRewardsState } from '../../context/rewards_context'
import { useVpnState } from '../../context/vpn_context'
import { NtpWidget } from './ntp_widget'
import { RewardsWidget } from './rewards_widget'
import { TalkWidget } from './talk_widget'
import { VpnWidget } from './vpn_widget'
import { StatsWidget } from './stats_widget'
import { NewsWidget } from './news_widget'

import { style } from './widget_stack.style'

type TabName = 'rewards' | 'talk' | 'vpn' | 'stats' | 'news'

interface Props {
  name: string
  tabs: TabName[]
}

export function WidgetStack(props: Props) {
  const showTalkWidget = useNewTabState((s) => s.showTalkWidget)
  const talkFeatureEnabled = useNewTabState((s) => s.talkFeatureEnabled)
  const showShieldsStats = useNewTabState((s) => s.showShieldsStats)
  const showRewardsWidget = useRewardsState((s) => s.showRewardsWidget)
  const rewardsFeatureEnabled = useRewardsState((s) => s.rewardsFeatureEnabled)
  const vpnFeatureEnabled = useVpnState((s) => s.vpnFeatureEnabled)
  const showVpnWidget = useVpnState((s) => s.showVpnWidget)
  const showNews = useBraveNews().isShowOnNTPPrefEnabled
  const newsFeatureEnabled = useNewTabState((s) => s.newsFeatureEnabled)

  const [currentTab, setCurrentTab] = React.useState(loadCurrentTab(props.name))

  const visibleTabs = React.useMemo(() => {
    return props.tabs.filter((tab) => {
      switch (tab) {
        case 'rewards':
          return rewardsFeatureEnabled && showRewardsWidget
        case 'talk':
          return talkFeatureEnabled && showTalkWidget
        case 'vpn':
          return vpnFeatureEnabled && showVpnWidget
        case 'stats':
          return showShieldsStats
        case 'news':
          return newsFeatureEnabled && showNews
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
    showNews,
  ])

  function renderTabButton(tab: TabName) {
    return (
      <button
        key={tab}
        className={tab === activeTab ? 'active' : ''}
        onClick={() => {
          storeCurrentTab(props.name, tab)
          setCurrentTab(tab)
        }}
      >
        {renderProductIcon(tab)}
      </button>
    )
  }

  function renderProductIcon(tab: TabName) {
    switch (tab) {
      case 'rewards':
        return <Icon name='product-bat-outline' />
      case 'talk':
        return <Icon name='product-brave-talk' />
      case 'vpn':
        return <Icon name='product-vpn' />
      case 'stats':
        return <Icon name='bar-chart' />
      case 'news':
        return <Icon name='product-brave-news' />
    }
  }

  function renderWidget() {
    switch (activeTab) {
      case 'rewards':
        return <RewardsWidget />
      case 'talk':
        return <TalkWidget />
      case 'vpn':
        return <VpnWidget />
      case 'stats':
        return <StatsWidget />
      case 'news':
        return <NewsWidget />
    }
  }

  if (visibleTabs.length === 0) {
    return null
  }

  const activeTab =
    currentTab && visibleTabs.includes(currentTab) ? currentTab : visibleTabs[0]

  return (
    <NtpWidget>
      <div data-css-scope={style.scope}>
        {visibleTabs.length > 1 && (
          <div className='stack-tabs'>{visibleTabs.map(renderTabButton)}</div>
        )}
        <div className='widget'>{renderWidget()}</div>
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
