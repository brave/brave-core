// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'

import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import store from './store'
import * as WalletActions from '../common/actions/wallet_actions'
import Container from './container'

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()
  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])
  return (
    <Provider store={store}>
      {initialThemeType &&
      <BraveCoreThemeProvider
        initialThemeType={initialThemeType}
        dark={walletDarkTheme}
        light={walletLightTheme}
      >
        <Container />
      </BraveCoreThemeProvider>
      }
    </Provider>
  )
}

function initialize () {
  store.dispatch(WalletActions.initialize())
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
