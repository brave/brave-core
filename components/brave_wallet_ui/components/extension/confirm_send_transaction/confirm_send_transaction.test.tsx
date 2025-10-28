// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Utils
import {
  // eslint-disable-next-line import/no-named-default
  default as BraveCoreThemeProvider,
} from '../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../utils/test-utils'
import {
  deserializeTransaction, //
} from '../../../utils/model-serialization-utils'

// Types
import { WalletApiDataOverrides } from '../../../constants/testing_types'

// Mocks
import { mockAccount, mockEthAccount } from '../../../common/constants/mocks'
import { mockEthMainnet } from '../../../stories/mock-data/mock-networks'

// Components
import { ConfirmSendTransaction } from './confirm_send_transaction'
import {
  mockETHNativeTokenSendTransaction, //
} from '../../../stories/mock-data/mock-transaction-info'
import {
  mockUniswapOriginInfo, //
} from '../../../stories/mock-data/mock-origin-info'

// Helper function to render the component with any transaction
const renderWithTransaction = (transaction: any) => {
  const mockApiData: WalletApiDataOverrides = {
    transactionInfos: [deserializeTransaction(transaction)],
    networks: [mockEthMainnet],
    accountInfos: [mockAccount, mockEthAccount],
  }

  const store = createMockStore(
    {
      uiStateOverride: {
        selectedPendingTransactionId: transaction.id,
      },
    },
    mockApiData,
  )
  return render(
    <Provider store={store}>
      <BraveCoreThemeProvider>
        <ConfirmSendTransaction />
      </BraveCoreThemeProvider>
    </Provider>,
  )
}

// Helper function to wait for loading panel to disappear
// Using 1000ms timeout since mocked data should load quickly
const waitForLoadingToComplete = async (timeout = 1000) => {
  await waitFor(
    () => {
      const loadingPanel = screen.queryByTestId('loading-panel')
      expect(loadingPanel).not.toBeInTheDocument()
    },
    { timeout },
  )
}

describe('ConfirmSendTransactionInternal', () => {
  const renderComponent = () =>
    renderWithTransaction(mockETHNativeTokenSendTransaction)

  it('should render the component', async () => {
    renderComponent()

    // Wait for the transaction details to load (loading panel to disappear)
    await waitForLoadingToComplete()

    // Verify origin info is not rendered for internal transactions
    expect(screen.getByText('braveWalletPanelTitle')).toBeInTheDocument()
    expect(screen.queryByTestId('origin-info-card')).toBeNull()

    // Verify strings are rendered
    expect(screen.getByText('braveWalletConfirmSend')).toBeInTheDocument()
    expect(screen.getByText('braveWalletSend')).toBeInTheDocument()
    expect(screen.getByText('braveWalletSwapTo')).toBeInTheDocument()

    // Verify amount is rendered
    expect(screen.getByText('12.5 ETH')).toBeInTheDocument()

    // Verify recipient is rendered
    expect(screen.getByText('mockEthAccountName')).toBeInTheDocument()
    expect(screen.getByText('0xf812***37Db')).toBeInTheDocument()

    // Verify network is rendered
    expect(screen.getByText('Ethereum Mainnet')).toBeInTheDocument()

    // Verify fee is rendered
    expect(screen.getByText('0.0000021 ETH')).toBeInTheDocument()
    expect(screen.getByText('$0.008135')).toBeInTheDocument()
  })
})

const mockDAppSendTransaction = {
  ...mockETHNativeTokenSendTransaction,
  originInfo: mockUniswapOriginInfo,
}

describe('ConfirmSendTransactionDApp', () => {
  const renderComponent = () => renderWithTransaction(mockDAppSendTransaction)

  it('should render origin-info-card for dapp transactions', async () => {
    renderComponent()

    // Wait for the transaction details to load
    await waitForLoadingToComplete()

    // Verify origin info IS rendered for dapp transactions
    expect(screen.queryByTestId('origin-info-card')).toBeInTheDocument()
    expect(screen.getByText('Uniswap NFT Aggregator')).toBeInTheDocument()
    expect(screen.getByText('https://app.')).toBeInTheDocument()
    expect(screen.getByText('uniswap.org')).toBeInTheDocument()
    expect(screen.getByText('braveWalletVerified')).toBeInTheDocument()
  })
})
