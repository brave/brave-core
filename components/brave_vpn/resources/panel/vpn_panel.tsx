// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'

import BraveCoreThemeProvider from '../../../common/BraveCoreThemeProvider'
import vpnDarkTheme from './theme/vpn-dark'
import vpnLightTheme from './theme/vpn-light'
import Container from './container'
import { PanelWrapper } from './style'
import apiProxy from './vpn_panel_api_proxy.js'

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()

  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])

  return (
    <React.Fragment>
      {initialThemeType &&
        <BraveCoreThemeProvider
          initialThemeType={initialThemeType}
          dark={vpnDarkTheme}
          light={vpnLightTheme}
        >
          <PanelWrapper>
            <Container />
          </PanelWrapper>
        </BraveCoreThemeProvider>
      }
    </React.Fragment>
  )
}

function initialize () {
  render(<App />, document.getElementById('mountPoint'),
  () => {
    apiProxy.getInstance().showUI()
    apiProxy.getInstance().createVPNConnection()
  })
}

document.addEventListener('DOMContentLoaded', initialize)
