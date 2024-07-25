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

import { loadTimeData } from '../../common/loadTimeData'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import store, { walletPanelApiProxy } from './store'
import * as WalletActions from '../common/actions/wallet_actions'
import Container from './container'
import { ApiProxyContext } from '../common/context/api-proxy.context'
import {
  runLocalStorageMigrations //
} from '../common/constants/local-storage-keys'
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
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
  store.dispatch(WalletActions.initialize())
}

document.addEventListener('DOMContentLoaded', initialize)
