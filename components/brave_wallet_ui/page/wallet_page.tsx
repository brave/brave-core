// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { BrowserRouter } from 'react-router-dom'

// assets
import faveiconUrl from '../assets/svg-icons/brave-icon.svg'

// utils
import { loadTimeData } from '../../common/loadTimeData'
import {
  runLocalStorageMigrations //
} from '../common/constants/local-storage-keys'

// actions
import * as WalletActions from '../common/actions/wallet_actions'

// contexts
import { ApiProxyContext } from '../common/context/api-proxy.context'

// components
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import Container from './container'
import { store, walletPageApiProxy } from './store'

// style
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import 'emptykit.css'

import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome://resources/brave-icons')

function App() {
  React.useEffect(() => {
    /** Sets FAVICON for Wallet Pages */
    let link = document.querySelector("link[rel~='icon']") as HTMLLinkElement
    if (!link) {
      link = document.createElement('link')
      link.rel = 'icon'
      document.getElementsByTagName('head')[0].appendChild(link)
    }
    link.href = faveiconUrl
  }, [])

  React.useEffect(() => {
    runLocalStorageMigrations()
  }, [])

  return (
    <Provider store={store}>
      <BrowserRouter>
        <BraveCoreThemeProvider
          dark={walletDarkTheme}
          light={walletLightTheme}
        >
          <ApiProxyContext.Provider value={walletPageApiProxy}>
            <Container />
          </ApiProxyContext.Provider>
        </BraveCoreThemeProvider>
      </BrowserRouter>
    </Provider>
  )
}

function initialize() {
  initLocale(loadTimeData.data_)
  const root = createRoot(document.getElementById('root')!)
  root.render(<App />)
  store.dispatch(WalletActions.initialize())
}

document.addEventListener('DOMContentLoaded', initialize)
