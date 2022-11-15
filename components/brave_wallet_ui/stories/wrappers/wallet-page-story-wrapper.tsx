// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createStore, combineReducers } from 'redux'
import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

// utils
import { createSendCryptoReducer, PendingCryptoSendState } from '../../common/reducers/send_crypto_reducer'
import { createWalletReducer } from '../../common/slices/wallet.slice'
import { createPageReducer } from '../../page/reducers/page_reducer'
import { AccountsTabState, createAccountsTabReducer } from '../../page/reducers/accounts-tab-reducer'

// actions
import { WalletActions } from '../../common/actions'

// types
import { PageState, WalletRoutes, WalletState } from '../../constants/types'

// components
import { LibContext } from '../../common/context/lib.context'

// Mocks
import * as Lib from '../../common/async/__mocks__/lib'
import { mockPageState } from '../mock-data/mock-page-state'
import { mockWalletState } from '../mock-data/mock-wallet-state'
import { mockSendCryptoState } from '../mock-data/send-crypto-state'
import { ApiProxyContext } from '../../common/context/api-proxy.context'
import { getMockedAPIProxy } from '../../common/async/__mocks__/bridge'
import { mockAccountsTabState } from '../mock-data/mock-accounts-tab-state'

export interface WalletPageStoryProps {
  walletStateOverride?: Partial<WalletState>
  pageStateOverride?: Partial<PageState>
  accountTabStateOverride?: Partial<AccountsTabState>
  sendCryptoStateOverride?: Partial<PendingCryptoSendState>
}

const mockedProxy = getMockedAPIProxy()

export const WalletPageStory: React.FC<React.PropsWithChildren<WalletPageStoryProps>> = ({
  children,
  pageStateOverride,
  walletStateOverride,
  accountTabStateOverride,
  sendCryptoStateOverride
}) => {
  // redux
  const store = React.useMemo(() => {
    return createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        ...(walletStateOverride || {})
      }),
      page: createPageReducer({
        ...mockPageState,
        ...(pageStateOverride || {})
      }),
      sendCrypto: createSendCryptoReducer({
        ...mockSendCryptoState,
        ...(sendCryptoStateOverride || {})
      }),
      accountsTab: createAccountsTabReducer({
        ...mockAccountsTabState,
        ...(accountTabStateOverride || {})
      })
    }))
  }, [walletStateOverride, pageStateOverride])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize())
  }, [store])

  // render
  return (
    <MemoryRouter initialEntries={[WalletRoutes.OnboardingWelcome]}>
      <Provider store={store}>
        <ApiProxyContext.Provider value={mockedProxy}>
          <LibContext.Provider value={Lib as any}>
            {children}
          </LibContext.Provider>
        </ApiProxyContext.Provider>
      </Provider>
    </MemoryRouter>
  )
}

export default WalletPageStory
