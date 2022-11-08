// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

// utils
import { createSendCryptoReducer } from '../../common/reducers/send_crypto_reducer'
import { createWalletReducer } from '../../common/slices/wallet.slice'
import { createPanelReducer } from '../../panel/reducers/panel_reducer'

// actions
import { WalletActions } from '../../common/actions'

// types
import { PanelState, WalletState } from '../../constants/types'

// components
import { LibContext } from '../../common/context/lib.context'

// Mocks
import * as Lib from '../../common/async/__mocks__/lib'
import { mockWalletState } from '../mock-data/mock-wallet-state'
import { mockSendCryptoState } from '../mock-data/send-crypto-state'
import { mockPanelState } from '../mock-data/mock-panel-state'
import { ApiProxyContext } from '../../common/context/api-proxy.context'
import { getMockedAPIProxy } from '../../common/async/__mocks__/bridge'
import { configureStore } from '@reduxjs/toolkit'
import { walletApi } from '../../common/slices/api.slice'
import { createPageReducer } from '../../page/reducers/page_reducer'
import { mockPageState } from '../mock-data/mock-page-state'

const mockedProxy = getMockedAPIProxy()

export interface WalletPanelStoryProps {
  walletStateOverride?: Partial<WalletState>
  panelStateOverride?: Partial<PanelState>
}

export const WalletPanelStory: React.FC<React.PropsWithChildren<WalletPanelStoryProps>> = ({
  children,
  panelStateOverride,
  walletStateOverride
}) => {
  // redux
  const store = React.useMemo(() => {
    return configureStore({
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
        [walletApi.reducerPath]: walletApi.reducer,
        sendCrypto: createSendCryptoReducer(mockSendCryptoState)
      },
      devTools: true,
      middleware: (getDefaultMiddleware) => getDefaultMiddleware().concat(walletApi.middleware)
    })
  }, [walletStateOverride, panelStateOverride])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize())
  }, [store])

  // render
  return (
    <MemoryRouter initialEntries={['/']}>
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

export default WalletPanelStory
