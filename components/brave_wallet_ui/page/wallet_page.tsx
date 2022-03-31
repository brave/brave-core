// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { loadTimeData } from '../../common/loadTimeData'
import Container from './container'
import * as WalletActions from '../common/actions/wallet_actions'
import store from './store'
import { BrowserRouter } from 'react-router-dom'
import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'

import * as Lib from '../common/async/lib'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import { LibContext } from '../common/context/lib.context'

function App () {
  const [initialThemeType, setInitialThemeType] = React.useState<chrome.braveTheme.ThemeType>()
  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])
  return (
    <Provider store={store}>
      <BrowserRouter>
        {initialThemeType &&
          <BraveCoreThemeProvider
            initialThemeType={initialThemeType}
            dark={walletDarkTheme}
            light={walletLightTheme}
          >
            <LibContext.Provider value={Lib}>
              <Container />
            </LibContext.Provider>
          </BraveCoreThemeProvider>
        }
      </BrowserRouter>
    </Provider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  store.dispatch(WalletActions.initialize())
  render(<App />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
