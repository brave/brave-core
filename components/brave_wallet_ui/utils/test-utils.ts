// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { configureStore } from '@reduxjs/toolkit'

// types
import { WalletActions } from '../common/actions'
import { PanelState, WalletState } from '../constants/types'

// reducers
import { createSendCryptoReducer } from '../common/reducers/send_crypto_reducer'
import { createWalletApi } from '../common/slices/api.slice'
import { createWalletReducer } from '../common/slices/wallet.slice'
import { createPageReducer } from '../page/reducers/page_reducer'
import { createPanelReducer } from '../panel/reducers/panel_reducer'

// mocks
import { getMockedAPIProxy } from '../common/async/__mocks__/bridge'
import { mockPageState } from '../stories/mock-data/mock-page-state'
import { mockPanelState } from '../stories/mock-data/mock-panel-state'
import { mockWalletState } from '../stories/mock-data/mock-wallet-state'
import { mockSendCryptoState } from '../stories/mock-data/send-crypto-state'

export interface RootStateOverrides {
  walletStateOverride?: Partial<WalletState>
  panelStateOverride?: Partial<PanelState>
}

export const createMockStore = ({
  panelStateOverride,
  walletStateOverride
}: RootStateOverrides) => {
  // api
  const api = createWalletApi(getMockedAPIProxy)
  // redux
  const store = configureStore({
    reducer: {
      wallet: createWalletReducer({
        ...mockWalletState,
        ...(walletStateOverride || {})
      }),
      panel: createPanelReducer({
        ...mockPanelState,
        ...(panelStateOverride || {})
      }),
      page: createPageReducer(mockPageState),
      [api.reducerPath]: api.reducer,
      sendCrypto: createSendCryptoReducer(mockSendCryptoState)
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
