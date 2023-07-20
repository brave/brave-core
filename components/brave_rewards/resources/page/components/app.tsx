/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PlatformContext } from '../lib/platform_context'
import { LayoutManager } from './layout_manager'
import { useActions, useRewardsData } from '../lib/redux_hooks'
import { Settings } from './settings'
import { AppErrorBoundary } from './app_error_boundary'

import * as style from './app.style'

export function App () {
  const { isAndroid } = React.useContext(PlatformContext)

  const actions = useActions()
  const rewardsData = useRewardsData((data) => ({
    initializing: data.initializing
  }))

  React.useEffect(() => {
    if (rewardsData.initializing) {
      actions.isInitialized()
    }
  }, [rewardsData.initializing])

  const renderToolbar = React.useCallback((elem: HTMLElement | null) => {
    if (elem && !isAndroid) {
      const toolbar = document.createElement('cr-toolbar')
      toolbar.setAttribute('no-search', 'no-search')
      elem.appendChild(toolbar)
    }
  }, [])

  return (
    <LayoutManager>
      <style.root id='rewardsPage'>
        <div ref={renderToolbar}></div>
        <AppErrorBoundary>
          <Settings />
        </AppErrorBoundary>
      </style.root>
    </LayoutManager>
  )
}
