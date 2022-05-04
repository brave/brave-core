// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'

import '$web-components/app.global.scss'
import { loadTimeData } from '../../../common/loadTimeData'
import BraveCoreThemeProvider from '../../../common/BraveCoreThemeProvider'
import vpnDarkTheme from './theme/vpn-dark'
import vpnLightTheme from './theme/vpn-light'
import Container from './container'
import { PanelWrapper } from './style'
import store from './state/store'
import getPanelBrowserAPI from './api/panel_browser_api'

function App () {
  React.useEffect(() => {
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
    <Provider store={store}>
      <BraveCoreThemeProvider
        dark={vpnDarkTheme}
        light={vpnLightTheme}
      >
        <PanelWrapper>
          <Container />
        </PanelWrapper>
      </BraveCoreThemeProvider>
    </Provider>
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
