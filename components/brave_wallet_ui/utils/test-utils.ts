// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { configureStore } from '@reduxjs/toolkit'

// types
import { WalletActions } from '../common/actions'
import { PageState, PanelState, WalletState } from '../constants/types'

// reducers
import { createWalletApi } from '../common/slices/api.slice'
import { createWalletReducer } from '../common/slices/wallet.slice'
import { createPageReducer } from '../page/reducers/page_reducer'

// mocks
import {
  getMockedAPIProxy,
  WalletApiDataOverrides
} from '../common/async/__mocks__/bridge'
import { mockPageState } from '../stories/mock-data/mock-page-state'
import { mockWalletState } from '../stories/mock-data/mock-wallet-state'
import { AccountsTabState, createAccountsTabReducer } from '../page/reducers/accounts-tab-reducer'
import { mockAccountsTabState } from '../stories/mock-data/mock-accounts-tab-state'

export interface RootStateOverrides {
  accountTabStateOverride?: Partial<AccountsTabState>
  pageStateOverride?: Partial<PageState>
  panelStateOverride?: Partial<PanelState>
  walletStateOverride?: Partial<WalletState>
}

export const createMockStore = (
  {
    accountTabStateOverride,
    pageStateOverride,
    panelStateOverride,
    walletStateOverride
  }: RootStateOverrides,
  apiOverrides?: WalletApiDataOverrides
) => {
  // api
  getMockedAPIProxy().applyOverrides(apiOverrides)
  const api = createWalletApi(getMockedAPIProxy)
  // redux
  const store = configureStore({
    reducer: {
      wallet: createWalletReducer({
        ...mockWalletState,
        ...(walletStateOverride || {})
      }),
      page: createPageReducer({
        ...mockPageState,
        ...(pageStateOverride || {})
      }),
      accountsTab: createAccountsTabReducer({
        ...mockAccountsTabState,
        ...(accountTabStateOverride || {})
      }),
      [api.reducerPath]: api.reducer
    },
    devTools: true,
    middleware: (getDefaultMiddleware) => getDefaultMiddleware().concat(api.middleware)
  })

  const proxy = getMockedAPIProxy()
  proxy?.addJsonRpcServiceObserver?.(store)
  proxy?.addKeyringServiceObserver?.(store)
  proxy?.addTxServiceObserver?.(store)
  proxy?.addBraveWalletServiceObserver?.(store)
  store.dispatch(WalletActions.initialize())

  return store
}
