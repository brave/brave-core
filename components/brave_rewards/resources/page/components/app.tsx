/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LayoutManager } from './layout_manager'
import { useActions, useRewardsData } from '../lib/redux_hooks'
import { Settings } from './settings'
import { AppErrorBoundary } from './app_error_boundary'

export function App () {
  const actions = useActions()
  const rewardsData = useRewardsData((data) => ({
    initializing: data.initializing,
    adsData: data.adsData,
    enabledAdsMigrated: data.enabledAdsMigrated
  }))

  React.useEffect(() => {
    if (rewardsData.initializing) {
      actions.isInitialized()

      if (!rewardsData.enabledAdsMigrated) {
        const { adsEnabled, adsIsSupported } = rewardsData.adsData
        if (adsIsSupported) {
          actions.onAdsSettingSave('adsEnabledMigrated', adsEnabled)
        }
      }
    }
  }, [rewardsData.initializing])

  return (
    <LayoutManager>
      <div id='rewardsPage'>
        <AppErrorBoundary>
          {!rewardsData.initializing && <Settings />}
        </AppErrorBoundary>
      </div>
    </LayoutManager>
  )
}
