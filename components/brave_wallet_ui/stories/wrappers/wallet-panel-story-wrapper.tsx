// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { createStore, combineReducers } from 'redux'
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
    return createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        ...(walletStateOverride || {})
      }),
      panel: createPanelReducer({
        ...mockPanelState,
        ...(panelStateOverride || {})
      }),
      sendCrypto: createSendCryptoReducer(mockSendCryptoState)
    }))
  }, [walletStateOverride, panelStateOverride])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize())
  }, [store])

  // render
  return (
    <MemoryRouter initialEntries={['/']}>
      <Provider store={store}>
        <LibContext.Provider value={Lib as any}>
          {children}
        </LibContext.Provider>
      </Provider>
    </MemoryRouter>
  )
}

export default WalletPanelStory
