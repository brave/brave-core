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
import BraveCoreThemeProvider from '../../../../common/BraveCoreThemeProvider'

describe('NetworkInfo', () => {
  it('should render network information correctly', () => {
    const { container } = render(
      <BraveCoreThemeProvider>
        <NetworkInfo network={mockBNBChainNetwork} />
      </BraveCoreThemeProvider>,
    )

    // Check if the component renders
    expect(container).toBeInTheDocument()

    // Check if all network information sections are displayed
    expect(
      screen.getByText('braveWalletAllowAddNetworkName:'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletAllowAddNetworkUrl:'),
    ).toBeInTheDocument()
    expect(screen.getByText('braveWalletChainId:')).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletAllowAddNetworkCurrencySymbol:'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletWatchListTokenDecimals:'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletAllowAddNetworkExplorer:'),
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
