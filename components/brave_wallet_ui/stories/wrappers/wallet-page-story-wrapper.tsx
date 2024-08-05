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

// theme
import LightDarkThemeProvider from '../../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../../theme/wallet-dark'
import walletLightTheme from '../../theme/wallet-light'

// Mocks
import { WalletApiDataOverrides } from '../../constants/testing_types'
import '../locale'

export interface WalletPageStoryProps {
  walletStateOverride?: Partial<WalletState>
  pageStateOverride?: Partial<PageState>
  accountTabStateOverride?: Partial<AccountsTabState>
  uiStateOverride?: Partial<UIState>
  apiOverrides?: WalletApiDataOverrides
  initialRoute?: WalletRoutes
}

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
    uiStateOverride,
    apiOverrides
  ])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize())
  }, [store])

  // render
  return (
    <LightDarkThemeProvider
      initialThemeType={'Light'}
      dark={walletDarkTheme}
      light={walletLightTheme}
    >
      <MemoryRouter
        initialEntries={[initialRoute || WalletRoutes.OnboardingWelcome]}
      >
        <Provider store={store}>
            {children}
        </Provider>
      </MemoryRouter>
    </LightDarkThemeProvider>
  )
}

export default WalletPageStory
