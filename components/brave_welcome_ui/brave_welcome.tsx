// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'
import { addWebUIListener } from 'chrome://resources/js/cr.m.js'

import { loadTimeData } from '$web-common/loadTimeData'
import darkTheme from './theme/welcome-dark'
import lightTheme from './theme/welcome-light'

import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'

import MainContainer from './main_container'
import DataContext from './state/context'
import { ViewType } from './state/component_types'
import { useInitializeImportData, useProfileCount } from './state/hooks'

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()
  const [viewType, setViewType] = React.useState<ViewType>(ViewType.Default)
  const [currentSelectedBrowser, setCurrentSelectedBrowser] = React.useState<string | undefined>(undefined)
  const { browserProfiles } = useInitializeImportData()
  const { profileCountRef, incrementCount, decrementCount } = useProfileCount()

  const store = {
    setViewType,
    setCurrentSelectedBrowser,
    incrementCount,
    browserProfiles,
    currentSelectedBrowser,
    viewType
  }

  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)

    addWebUIListener('brave-import-data-status-changed', (status: any) => {
      // TODO(tali): Handle item based events

      if (status.event === 'ImportStarted' && (profileCountRef.current > 0)) {
        setViewType(ViewType.ImportInProgress)
      }

      if (status.event === 'ImportEnded') {
        decrementCount()
        if (profileCountRef.current === 0) {
          setViewType(ViewType.ImportSucceeded)
        }
      }
    })
  }, [])

  return (
    <>
      {initialThemeType &&
        <DataContext.Provider
          value={store}
        >
          <BraveCoreThemeProvider
            initialThemeType={initialThemeType}
            dark={darkTheme}
            light={lightTheme}
          >
            <MainContainer />
          </BraveCoreThemeProvider>
        </DataContext.Provider>
      }
    </>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('root'),
  () => {})
}

document.addEventListener('DOMContentLoaded', initialize)
