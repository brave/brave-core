// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter } from 'react-router-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { setIconBasePath } from '@brave/leo/react/icon'

// utils
import type { BraveWallet } from '../constants/types'
import { loadTimeData } from '../../common/loadTimeData'
import getWalletPanelApiProxy from './wallet_panel_api_proxy'
import {
  runLocalStorageMigrations //
} from '../common/constants/local-storage-keys'

// theme
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// redux
import store, { walletPanelApiProxy } from './store'
import { refreshWalletInfo, visibilityChanged } from '../common/async/thunks'
import { navigateTo, showConnectToSite } from './async/wallet_panel_thunks'

// contexts
import { ApiProxyContext } from '../common/context/api-proxy.context'

// components
import Container from './container'

setIconBasePath('chrome://resources/brave-icons')

function App() {
  const [initialThemeType, setInitialThemeType] =
    React.useState<chrome.braveTheme.ThemeType>()
  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])

  React.useEffect(() => {
    runLocalStorageMigrations()
  }, [])

  return (
    <Provider store={store}>
      {initialThemeType && (
        <BraveCoreThemeProvider
          initialThemeType={initialThemeType}
          dark={walletDarkTheme}
          light={walletLightTheme}
        >
          <ApiProxyContext.Provider value={walletPanelApiProxy}>
            <BrowserRouter>
              <Container />
            </BrowserRouter>
          </ApiProxyContext.Provider>
        </BraveCoreThemeProvider>
      )}
    </Provider>
  )
}

function initialize() {
  initLocale(loadTimeData.data_)
  const apiProxy = getWalletPanelApiProxy()

  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)

  store.dispatch(refreshWalletInfo())

  // Setup external events
  document.addEventListener('visibilitychange', () => {
    store.dispatch(visibilityChanged(document.visibilityState === 'visible'))
  })

  // Parse webUI URL, dispatch showConnectToSite action if needed.
  // TODO(jocelyn): Extract ConnectToSite UI pieces out from panel UI.
  const url = new URL(window.location.href)
  if (url.hash === '#connectWithSite') {
    const accounts = url.searchParams.getAll('addr') || []
    const originSpec = url.searchParams.get('origin-spec') || ''
    const eTldPlusOne = url.searchParams.get('etld-plus-one') || ''
    const originInfo: BraveWallet.OriginInfo = {
      originSpec: originSpec,
      eTldPlusOne: eTldPlusOne
    }

    store.dispatch(showConnectToSite({ accounts, originInfo }))
    return
  }

  if (url.hash === '#approveTransaction') {
    // When this panel is explicitly selected we close the panel
    // UI after all transactions are approved or rejected.
    store.dispatch(navigateTo('approveTransaction'))
    apiProxy.panelHandler?.showUI()
    return
  }

  apiProxy?.panelHandler?.showUI()
}

document.addEventListener('DOMContentLoaded', initialize)
