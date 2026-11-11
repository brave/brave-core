// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../utils/test-utils'
import { mockUniswapOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { mockBasicAttentionToken } from '../../../stories/mock-data/mock-asset-options'

// Components
import { AddSuggestedTokenPanel } from './add_suggested_token_panel'

const mockSuggestToken: BraveWallet.AddSuggestTokenRequest = {
  dappOriginInfo: mockUniswapOriginInfo,
  token: mockBasicAttentionToken,
}

describe('AddSuggestedTokenPanel', () => {
  it('should render add suggested token panel correctly', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <AddSuggestedTokenPanel
            suggestToken={mockSuggestToken}
            requestsLength={1}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()
      // Token Name
      expect(screen.getByText('Basic Attention Token')).toBeInTheDocument()

      // Panel Title
      expect(
        screen.getByText(S.BRAVE_WALLET_ADD_SUGGESTED_TOKEN_TITLE),
      ).toBeInTheDocument()

      // Panel Description
      expect(
        screen.getByText(S.BRAVE_WALLET_ADD_SUGGESTED_TOKEN_DESCRIPTION),
      ).toBeInTheDocument()

      // Token Description
      expect(
        screen.getByText(S.BRAVE_WALLET_PORTFOLIO_ASSET_NETWORK_DESCRIPTION),
      ).toBeInTheDocument()

      // Buttons
      expect(screen.getByText(S.BRAVE_WALLET_BUTTON_CANCEL)).toBeInTheDocument()
      expect(screen.getByText(S.BRAVE_WALLET_ADD_TOKEN)).toBeInTheDocument()
    })
  })
})
