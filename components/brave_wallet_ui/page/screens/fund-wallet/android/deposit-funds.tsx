// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { BrowserRouter } from 'react-router-dom'

import { initLocale } from 'brave-ui'

// style
import walletDarkTheme from '../../../../theme/wallet-dark'
import walletLightTheme from '../../../../theme/wallet-light'
import 'emptykit.css'

// Utils
import { loadTimeData } from '../../../../../common/loadTimeData'
import * as Lib from '../../../../common/async/lib'

// actions
import * as WalletActions from '../../../../common/actions/wallet_actions'

// Components
import { store, walletPageApiProxy } from '../../../store'
import BraveCoreThemeProvider
  from '../../../../../common/BraveCoreThemeProvider'
import { DepositFundsScreen } from '../deposit-funds'
import { LibContext } from '../../../../common/context/lib.context'
import { ApiProxyContext }
  from '../../../../common/context/api-proxy.context'

import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome://resources/brave-icons')

export function AndroidDepositApp() {
  return (
    <Provider store={store}>
      <BrowserRouter>
        <BraveCoreThemeProvider dark={walletDarkTheme} light={walletLightTheme}>
          <ApiProxyContext.Provider value={walletPageApiProxy}>
            <LibContext.Provider value={Lib}>
              <DepositFundsScreen />
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
  render(<AndroidDepositApp />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
