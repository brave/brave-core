// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

// utils

// actions
import { WalletActions } from '../../common/actions'

// types
import { PanelState, UIState, WalletState } from '../../constants/types'

// components
import { LibContext } from '../../common/context/lib.context'

// Mocks
import * as Lib from '../../common/async/__mocks__/lib'
import { ApiProxyContext } from '../../common/context/api-proxy.context'
import {
  getMockedAPIProxy,
  WalletApiDataOverrides
} from '../../common/async/__mocks__/bridge'
import { createMockStore } from '../../utils/test-utils'

const mockedProxy = getMockedAPIProxy()

export interface WalletPanelStoryProps {
  walletStateOverride?: Partial<WalletState>
  panelStateOverride?: Partial<PanelState>
  uiStateOverride?: Partial<UIState>
  walletApiDataOverrides?: WalletApiDataOverrides
}

export const WalletPanelStory: React.FC<React.PropsWithChildren<WalletPanelStoryProps>> = ({
  children,
  panelStateOverride,
  walletStateOverride,
  uiStateOverride,
  walletApiDataOverrides
}) => {
  // redux
  const store = React.useMemo(() => {
    return createMockStore(
      {
        walletStateOverride,
        panelStateOverride,
        uiStateOverride: uiStateOverride
      },
      walletApiDataOverrides
    )
  }, [
    walletStateOverride,
    panelStateOverride,
    walletApiDataOverrides,
    uiStateOverride
  ])

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
