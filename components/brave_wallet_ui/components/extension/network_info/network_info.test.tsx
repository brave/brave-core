// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'

// Components
import { NetworkInfo } from './network_info'

// Mocks
import { mockBNBChainNetwork } from '../../../stories/mock-data/mock-networks'
import { WalletTestThemeProvider } from '../../../utils/test-utils'

describe('NetworkInfo', () => {
  it('should render network information correctly', () => {
    const { container } = render(
      <WalletTestThemeProvider>
        <NetworkInfo network={mockBNBChainNetwork} />
      </WalletTestThemeProvider>,
    )

    // Check if the component renders
    expect(container).toBeInTheDocument()

    // Check if all network information sections are displayed
    expect(
      screen.getByText('BRAVE_WALLET_ALLOW_ADD_NETWORK_NAME:'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('BRAVE_WALLET_ALLOW_ADD_NETWORK_URL:'),
    ).toBeInTheDocument()
    expect(screen.getByText('BRAVE_WALLET_CHAIN_ID:')).toBeInTheDocument()
    expect(
      screen.getByText('BRAVE_WALLET_ALLOW_ADD_NETWORK_CURRENCY_SYMBOL:'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('BRAVE_WALLET_WATCH_LIST_TOKEN_DECIMALS:'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('BRAVE_WALLET_ALLOW_ADD_NETWORK_EXPLORER:'),
    ).toBeInTheDocument()

    // Check if network values are displayed
    expect(screen.getByText(mockBNBChainNetwork.chainName)).toBeInTheDocument()
    expect(
      screen.getByText(mockBNBChainNetwork.rpcEndpoints[0].url),
    ).toBeInTheDocument()
    expect(screen.getByText(mockBNBChainNetwork.chainId)).toBeInTheDocument()
    expect(screen.getByText(mockBNBChainNetwork.symbol)).toBeInTheDocument()
    expect(
      screen.getByText(mockBNBChainNetwork.decimals.toString()),
    ).toBeInTheDocument()
    expect(
      screen.getByText(mockBNBChainNetwork.blockExplorerUrls[0]),
    ).toBeInTheDocument()
  })
})
