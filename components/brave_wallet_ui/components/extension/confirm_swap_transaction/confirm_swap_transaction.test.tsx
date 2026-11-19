// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'
import { WalletApiDataOverrides } from '../../../constants/testing_types'

// Utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../utils/test-utils'

// Mocks
import { mockAccount, mockEthAccount } from '../../../common/constants/mocks'
import { mockEthMainnet } from '../../../stories/mock-data/mock-networks'
import {
  mockETHSwapTransaction,
  mockETHTokenSwapWithNativeValueTransaction,
} from '../../../stories/mock-data/mock-transaction-info'

// Components
import { ConfirmSwapTransaction } from './confirm_swap_transaction'

const renderWithTransaction = (transaction: BraveWallet.TransactionInfo) => {
  const mockApiData: WalletApiDataOverrides = {
    transactionInfos: [transaction],
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
      <WalletTestThemeProvider>
        <ConfirmSwapTransaction />
      </WalletTestThemeProvider>
    </Provider>,
  )
}

const waitForLoadingToComplete = async (timeout = 1000) => {
  await waitFor(
    () => {
      expect(screen.queryByTestId('loading-panel')).not.toBeInTheDocument()
    },
    { timeout },
  )
}

describe('ConfirmSwapTransaction', () => {
  it('shows the native value sent along with a token swap', async () => {
    renderWithTransaction(mockETHTokenSwapWithNativeValueTransaction)
    await waitForLoadingToComplete()

    expect(screen.getByText(S.BRAVE_WALLET_VALUE)).toBeInTheDocument()
    expect(screen.getByText('0.09848 ETH')).toBeInTheDocument()
  })

  it('shows the native value when it differs from the native spend', async () => {
    renderWithTransaction({
      ...mockETHSwapTransaction,
      swapInfo: {
        ...mockETHSwapTransaction.swapInfo!,
        sourceAmount: '0x5af3107a4000', // 0.0001 ETH
      },
    })
    await waitForLoadingToComplete()

    expect(screen.getByText(S.BRAVE_WALLET_VALUE)).toBeInTheDocument()
    expect(screen.getByText('0.09848 ETH')).toBeInTheDocument()
  })

  it('hides the native value when it is the native spend', async () => {
    renderWithTransaction(mockETHSwapTransaction)
    await waitForLoadingToComplete()

    expect(screen.queryByText(S.BRAVE_WALLET_VALUE)).not.toBeInTheDocument()
  })
})
