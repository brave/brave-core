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

// theme
import LightDarkThemeProvider from '../../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../../theme/wallet-dark'
import walletLightTheme from '../../theme/wallet-light'

// Mocks
import { createMockStore } from '../../utils/test-utils'
import { WalletApiDataOverrides } from '../../constants/testing_types'
import '../locale'

// Styles
import { PanelWrapper } from './wallet_story_wrapper.style'

export interface WalletPanelStoryProps {
  walletStateOverride?: Partial<WalletState>
  panelStateOverride?: Partial<PanelState>
  uiStateOverride?: Partial<UIState>
  walletApiDataOverrides?: WalletApiDataOverrides
  dontWrapInPanelFrame?: boolean
}

export const WalletPanelStory: React.FC<
  React.PropsWithChildren<WalletPanelStoryProps>
> = ({
  children,
  panelStateOverride,
  walletStateOverride,
  uiStateOverride,
  walletApiDataOverrides,
  dontWrapInPanelFrame,
}) => {
  // redux
  const store = React.useMemo(() => {
    return createMockStore(
      {
        walletStateOverride,
        panelStateOverride,
        uiStateOverride: uiStateOverride,
      },
      walletApiDataOverrides,
    )
  }, [
    walletStateOverride,
    panelStateOverride,
    walletApiDataOverrides,
    uiStateOverride,
  ])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize())
  }, [store])

  // render
  return (
    <LightDarkThemeProvider
      dark={walletDarkTheme}
      light={walletLightTheme}
    >
      <MemoryRouter initialEntries={['/']}>
        <Provider store={store}>
          {dontWrapInPanelFrame ? (
            children
          ) : (
            <PanelWrapper>{children}</PanelWrapper>
          )}
        </Provider>
      </MemoryRouter>
    </LightDarkThemeProvider>
  )
}

export default WalletPanelStory
