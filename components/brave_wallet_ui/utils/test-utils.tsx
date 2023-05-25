// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { configureStore } from '@reduxjs/toolkit'
import { Provider } from 'react-redux'

// types
import { WalletActions } from '../common/actions'
import {
  PageState,
  PanelState,
  UIState,
  WalletState
} from '../constants/types'

// reducers
import { createWalletApi } from '../common/slices/api.slice'
import { createWalletReducer } from '../common/slices/wallet.slice'
import { createPageReducer } from '../page/reducers/page_reducer'
import { createUIReducer } from '../common/slices/ui.slice'

// mocks
import {
  getMockedAPIProxy,
  WalletApiDataOverrides
} from '../common/async/__mocks__/bridge'
import { mockPageState } from '../stories/mock-data/mock-page-state'
import { mockWalletState } from '../stories/mock-data/mock-wallet-state'
import {
  AccountsTabState,
  createAccountsTabReducer
} from '../page/reducers/accounts-tab-reducer'
import {
  mockAccountsTabState //
} from '../stories/mock-data/mock-accounts-tab-state'
import { mockUiState } from '../stories/mock-data/mock-ui-state'

export interface RootStateOverrides {
  accountTabStateOverride?: Partial<AccountsTabState>
  pageStateOverride?: Partial<PageState>
  panelStateOverride?: Partial<PanelState>
  walletStateOverride?: Partial<WalletState>
  uiStateOverride?: Partial<UIState>
}

export const createMockStore = (
  {
    accountTabStateOverride,
    pageStateOverride,
    panelStateOverride,
    uiStateOverride,
    walletStateOverride
  }: RootStateOverrides,
  apiOverrides?: WalletApiDataOverrides
) => {
  // api
  const mockedApiProxy = getMockedAPIProxy()
  mockedApiProxy.applyOverrides(apiOverrides)
  const api = createWalletApi(() => mockedApiProxy)
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
      ui: createUIReducer({
        ...mockUiState,
        ...(uiStateOverride || {})
      }),
      [api.reducerPath]: api.reducer
    },
    devTools: true,
    middleware: (getDefaultMiddleware) =>
      getDefaultMiddleware().concat(api.middleware)
  })

  const proxy = getMockedAPIProxy()
  proxy?.addJsonRpcServiceObserver?.(store)
  proxy?.addKeyringServiceObserver?.(store)
  proxy?.addTxServiceObserver?.(store)
  proxy?.addBraveWalletServiceObserver?.(store)
  store.dispatch(WalletActions.initialize())

  return store
}

export function renderHookOptionsWithMockStore(
  store: ReturnType<typeof createMockStore>
) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) => (
      <Provider store={store}>{children}</Provider>
    )
  }
}
