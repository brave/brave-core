// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { configureStore } from '@reduxjs/toolkit'
import { Provider } from 'react-redux'

// types
import { WalletActions } from '../common/actions'
import { PageState, PanelState, UIState, WalletState } from '../constants/types'

// reducers
import { createWalletApi } from '../common/slices/api.slice'
import { createWalletReducer } from '../common/slices/wallet.slice'
import { createPageReducer } from '../page/reducers/page_reducer'
import { createUIReducer } from '../common/slices/ui.slice'
import { createPanelReducer } from '../panel/reducers/panel_reducer'

// mocks
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
import { mockPanelState } from '../stories/mock-data/mock-panel-state'
import { resetCache } from '../common/async/base-query-cache'
import {
  makeBraveWalletServiceObserver,
  makeJsonRpcServiceObserver,
  makeKeyringServiceObserver,
  makeTxServiceObserver
} from '../common/wallet_api_proxy_observers'
import getAPIProxy, { resetAPIProxy } from '../common/async/bridge'
import { resetRewardsProxy } from '../common/async/brave_rewards_api_proxy'
import {
  BraveRewardsProxyOverrides,
  WalletApiDataOverrides
} from '../constants/testing_types'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

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
  apiOverrides?: WalletApiDataOverrides,
  rewardsApiOverrides?: BraveRewardsProxyOverrides
) => {
  // api reset
  resetCache() // clear base query cache
  resetRewardsProxy(rewardsApiOverrides)
  resetAPIProxy(apiOverrides)

  const api = createWalletApi()
  // redux
  const store = configureStore({
    reducer: {
      wallet: createWalletReducer({
        ...mockWalletState,
        ...(walletStateOverride || {})
      }),
      panel: createPanelReducer({
        ...mockPanelState,
        ...(panelStateOverride ?? {})
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

  const proxy = getAPIProxy()
  proxy?.addJsonRpcServiceObserver?.(makeJsonRpcServiceObserver(store))
  proxy?.addKeyringServiceObserver?.(makeKeyringServiceObserver(store))
  proxy?.addTxServiceObserver?.(makeTxServiceObserver(store))
  proxy?.addBraveWalletServiceObserver?.(makeBraveWalletServiceObserver(store))
  store.dispatch(WalletActions.initialize())

  return store
}

export function renderHookOptionsWithMockStore(
  store: ReturnType<typeof createMockStore>
) {
  return {
    wrapper: ({ children }: { children?: React.ReactNode }) => (
      <Provider store={store}>{children}</Provider>
    )
  }
}

export function renderComponentOptionsWithMockStore(
  store: ReturnType<typeof createMockStore>
) {
  return {
    wrapper: ({ children }: { children?: React.ReactNode }) => (
      <BraveCoreThemeProvider>
        <Provider store={store}>{children}</Provider>
      </BraveCoreThemeProvider>
    )
  }
}
