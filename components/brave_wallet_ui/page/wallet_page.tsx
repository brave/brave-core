// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { BrowserRouter } from 'react-router-dom'
import { PersistGate } from 'redux-persist/integration/react'

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
import { persistor, store, walletPageApiProxy } from './store'
import LoadingSkeleton from '../components/shared/loading-skeleton'

// style
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import 'emptykit.css'

import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome://resources/brave-icons')

const onPageRefreshInitiated = function (
  event: KeyboardEvent
): void {
  // listen for "Cmd/Ctrl + Shift + R"
  if (
    (event.ctrlKey || event.metaKey) &&
    event.shiftKey &&
    event.code === 'KeyR'
  ) {
    // IIFE wrapped because return type of `void` is expected
    ;(async () => {
      try {
        await persistor.purge()
      } catch (error) {
        console.error(`failed to purge persisted storage: ${error}`)
      }
    })()
  }
}

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()
  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])

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
    // clear the cache on force-refresh of page
    document.addEventListener('keydown', onPageRefreshInitiated)

    // cleanup
    return () => {
      document.removeEventListener('keydown', onPageRefreshInitiated)
    }
  }, [])

  return (
    <Provider store={store}>
      <BrowserRouter>
        {initialThemeType && (
          <BraveCoreThemeProvider
            initialThemeType={initialThemeType}
            dark={walletDarkTheme}
            light={walletLightTheme}
          >
            <ApiProxyContext.Provider value={walletPageApiProxy}>
              <LibContext.Provider value={Lib}>
                <PersistGate
                  persistor={persistor}
                  loading={<LoadingSkeleton width={'80%'} height={'80%'} />}
                >
                  <Container />
                </PersistGate>
              </LibContext.Provider>
            </ApiProxyContext.Provider>
          </BraveCoreThemeProvider>
        )}
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
