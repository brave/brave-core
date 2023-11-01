// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

// utils
import { AccountsTabState } from '../../page/reducers/accounts-tab-reducer'
import { createMockStore } from '../../utils/test-utils'

// actions
import { WalletActions } from '../../common/actions'

// types
import {
  PageState,
  WalletRoutes,
  WalletState,
  UIState
} from '../../constants/types'

// components
import { LibContext } from '../../common/context/lib.context'

// Mocks
import * as Lib from '../../common/async/__mocks__/lib'
import { ApiProxyContext } from '../../common/context/api-proxy.context'
import {
  WalletApiDataOverrides,
  getMockedAPIProxy
} from '../../common/async/__mocks__/bridge'

export interface WalletPageStoryProps {
  walletStateOverride?: Partial<WalletState>
  pageStateOverride?: Partial<PageState>
  accountTabStateOverride?: Partial<AccountsTabState>
  uiStateOverride?: Partial<UIState>
  apiOverrides?: WalletApiDataOverrides
  initialRoute?: WalletRoutes
}

const mockedProxy = getMockedAPIProxy()

export const WalletPageStory: React.FC<
  React.PropsWithChildren<WalletPageStoryProps>
> = ({
  children,
  pageStateOverride,
  walletStateOverride,
  accountTabStateOverride,
  uiStateOverride,
  apiOverrides,
  initialRoute
}) => {
  // redux
  const store = React.useMemo(() => {
    return createMockStore(
      {
        accountTabStateOverride,
        pageStateOverride,
        walletStateOverride,
        uiStateOverride
      },
      apiOverrides
    )
  }, [
    accountTabStateOverride,
    pageStateOverride,
    walletStateOverride,
    uiStateOverride
  ])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize({}))
  }, [store])

  // render
  return (
    <MemoryRouter
      initialEntries={[initialRoute || WalletRoutes.OnboardingWelcome]}
    >
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
