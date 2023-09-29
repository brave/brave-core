// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { BrowserRouter } from 'react-router-dom'

// assets
import faveiconUrl from '../assets/svg-icons/brave-icon.svg'

// utils
import { loadTimeData } from '../../common/loadTimeData'
import * as Lib from '../common/async/lib'
import { removeDeprecatedLocalStorageKeys } from '../common/constants/local-storage-keys'

// actions
import * as WalletActions from '../common/actions/wallet_actions'

// contexts
import { LibContext } from '../common/context/lib.context'
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

function App () {

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
    removeDeprecatedLocalStorageKeys()
  }, [])

  return (
    <Provider store={store}>
      <BrowserRouter>
        <BraveCoreThemeProvider
          dark={walletDarkTheme}
          light={walletLightTheme}
        >
          <ApiProxyContext.Provider value={walletPageApiProxy}>
            <LibContext.Provider value={Lib}>
              <Container />
            </LibContext.Provider>
          </ApiProxyContext.Provider>
        </BraveCoreThemeProvider>
      </BrowserRouter>
    </Provider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  store.dispatch(WalletActions.initialize({}))
  render(<App />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
