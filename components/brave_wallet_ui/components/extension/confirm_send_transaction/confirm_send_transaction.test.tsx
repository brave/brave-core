// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import * as React from 'react'
import { fireEvent, render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import BraveCoreThemeProvider from '../../../../common/BraveCoreThemeProvider'
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

// Mock leo confirm/reject buttons so clicks invoke handlers in jsdom
jest.mock('../confirm_reject_buttons/confirm_reject_buttons', () => ({
  ConfirmRejectButtons: ({
    onConfirm,
    onReject,
  }: {
    onConfirm: () => void
    onReject: () => void
  }) => (
    <div>
      <button
        type='button'
        onClick={onReject}
      >
        braveWalletAllowSpendRejectButton
      </button>
      <button
        type='button'
        onClick={onConfirm}
      >
        braveWalletAllowSpendConfirmButton
      </button>
    </div>
  ),
}))

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

const mockNftContractAddress = '0xabcdefabcdefabcdefabcdefabcdefabcdefabcd'
const mockOperatorAddress = mockEthAccount.accountId.address

const mockSetApprovalForAllTransaction = {
  ...mockETHNativeTokenSendTransaction,
  id: 'set-approval-for-all-tx',
  originInfo: mockUniswapOriginInfo,
  txType: BraveWallet.TransactionType.ERC721SetApprovalForAll,
  txArgs: [mockOperatorAddress, '0x1'],
  txParams: ['address', 'bool'],
  effectiveRecipient: mockNftContractAddress,
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        chainId: '0x1',
        nonce: '',
        gasPrice: '0x5f5e100',
        gasLimit: '0x5208',
        to: mockNftContractAddress,
        value: '0x0',
        data: [],
        signOnly: false,
        signedTransaction: undefined,
      },
      maxPriorityFeePerGas: '',
      maxFeePerGas: '',
    },
  },
}

describe('ConfirmSendTransactionSetApprovalForAll', () => {
  const renderComponent = () =>
    renderWithTransaction(mockSetApprovalForAllTransaction)

  it('should show approval for all warning and hide send details', async () => {
    renderComponent()

    await waitForLoadingToComplete()

    expect(
      screen.getByText('braveWalletApprovalForAllWarningTitle'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletApprovalForAllWarningDescription'),
    ).toBeInTheDocument()
    expect(screen.queryByText('braveWalletSend')).not.toBeInTheDocument()
    expect(screen.queryByText('12.5 ETH')).not.toBeInTheDocument()
    expect(screen.getByText('braveWalletSwapTo')).toBeInTheDocument()
  })

  it('should show final warning before confirming', async () => {
    renderComponent()

    await waitForLoadingToComplete()

    fireEvent.click(screen.getByText('braveWalletAllowSpendConfirmButton'))

    await waitFor(() => {
      expect(
        screen.getByText('braveWalletApprovalForAllFinalWarningTitle'),
      ).toBeInTheDocument()
      expect(screen.getByText('braveWalletConfirmAnyway')).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletTransactionCancel'),
      ).toBeInTheDocument()
    })
  })
})
