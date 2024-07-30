// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { Provider } from 'react-redux'
import { BrowserRouter } from 'react-router-dom'

import { initLocale } from 'brave-ui'

// Style
import walletDarkTheme from '../../../../theme/wallet-dark'
import walletLightTheme from '../../../../theme/wallet-light'
import 'emptykit.css'

// Utils
import { loadTimeData } from '../../../../../common/loadTimeData'

// Actions
import * as WalletActions from '../../../../common/actions/wallet_actions'

// Components
import { store, walletPageApiProxy } from '../../../store'
import {
  // eslint-disable-next-line import/no-named-default
  default as BraveCoreThemeProvider
} from '../../../../../common/BraveCoreThemeProvider'
import { FundWalletScreen } from '../../fund-wallet/fund-wallet'

// Hooks
import { ApiProxyContext } from '../../../../common/context/api-proxy.context'

// Resources
import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome://resources/brave-icons')

export function AndroidFundWalletApp() {
  return (
    <Provider store={store}>
      <BrowserRouter>
        <BraveCoreThemeProvider
          dark={walletDarkTheme}
          light={walletLightTheme}
        >
          <ApiProxyContext.Provider value={walletPageApiProxy}>
            <FundWalletScreen isAndroid={true} />
          </ApiProxyContext.Provider>
        </BraveCoreThemeProvider>
      </BrowserRouter>
    </Provider>
  )
}

function initialize() {
  initLocale(loadTimeData.data_)
  store.dispatch(WalletActions.initialize())
  const root = createRoot(document.getElementById('root')!)
  root.render(<AndroidFundWalletApp />)
}

document.addEventListener('DOMContentLoaded', initialize)
