// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'

import { loadTimeData } from '../../../common/loadTimeData'
import BraveCoreThemeProvider from '../../../common/BraveCoreThemeProvider'
import shieldsDarkTheme from './theme/shields-dark'
import shieldsLightTheme from './theme/shields-light'
import { PanelWrapper } from './style'
import getPanelBrowserAPI from './api/panel_browser_api'
import Container from './container'
import { useSiteBlockInfoData, useSiteSettingsData } from './state/hooks'
import DataContext from './state/context'

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()
  const { siteBlockInfo } = useSiteBlockInfoData()
  const { siteSettings, getSiteSettings } = useSiteSettingsData()

  const store = {
    siteBlockInfo,
    siteSettings,
    getSiteSettings
  }

  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)

    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        getPanelBrowserAPI().panelHandler.showUI()
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return (
    <>
      {initialThemeType &&
        <DataContext.Provider
          value={store}
        >
          <BraveCoreThemeProvider
            initialThemeType={initialThemeType}
            dark={shieldsDarkTheme}
            light={shieldsLightTheme}
          >
            <PanelWrapper>
              <Container />
            </PanelWrapper>
          </BraveCoreThemeProvider>
        </DataContext.Provider>
      }
    </>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'),
  () => {
    getPanelBrowserAPI().panelHandler.showUI()
  })
}

document.addEventListener('DOMContentLoaded', initialize)
