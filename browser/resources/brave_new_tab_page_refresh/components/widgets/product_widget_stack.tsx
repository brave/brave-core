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

import { style } from './product_widget_stack.style'

type TabName = 'rewards' | 'talk'

const tabList: TabName[] = ['rewards', 'talk']

export function ProductWidgetStack() {
  const showTalkWidget = useAppState((s) => s.showTalkWidget)
  const showRewardsWidget = useAppState((s) => s.showRewardsWidget)
  const rewardsFeatureEnabled = useAppState((s) => s.rewardsFeatureEnabled)

  const [currentTab, setCurrentTab] = React.useState(loadCurrentTab())

  const visibleTabs = React.useMemo(() => {
    return tabList.filter((tab) => {
      switch (tab) {
        case 'rewards': return rewardsFeatureEnabled && showRewardsWidget
        case 'talk': return showTalkWidget
      }
    })
  }, [showTalkWidget, rewardsFeatureEnabled, showRewardsWidget])

  React.useEffect(() => {
    storeCurrentTab(currentTab)
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
    }
  }

  function renderWidget() {
    switch (activeTab) {
      case 'rewards': return <RewardsWidget />
      case 'talk': return <TalkWidget />
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

const currentTabStorageKey = 'ntp-current-product-widget'

function loadCurrentTab(): TabName | null {
  const value = localStorage.getItem(currentTabStorageKey)
  switch (value) {
    case 'talk':
      return value
    default:
      return null
  }
}

function storeCurrentTab(tab: TabName | null) {
  localStorage.setItem(currentTabStorageKey, tab ?? '')
}
