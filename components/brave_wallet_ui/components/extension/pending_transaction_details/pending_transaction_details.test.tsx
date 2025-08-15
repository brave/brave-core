// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'

// Component
import {
  PendingTransactionDetails, //
} from './pending_transaction_details'

// Utils
import {
  createMockStore,
  renderComponentOptionsWithMockStore,
} from '../../../utils/test-utils'
import {
  getTypedSolanaTxInstructions, //
} from '../../../utils/solana-instruction-utils'

// Mocks
import {
  mockTransactionInfo,
  mockSOLTXInstructions,
  mockBtcSendTransaction,
} from '../../../stories/mock-data/mock-transaction-info'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
} from '../../../constants/types'

describe('PendingTransactionDetails', () => {
  const createMockStoreWithAccounts = () => {
    return createMockStore({})
  }

  describe('BTC/ZEC/Cardano Transactions', () => {
    it('should render BTC transaction inputs and outputs', () => {
      const store = createMockStore({})
      const renderOptions = renderComponentOptionsWithMockStore(store)

      render(
        <PendingTransactionDetails transactionInfo={mockBtcSendTransaction} />,
        renderOptions,
      )

      // Check locale exists
      expect(screen.getByText('braveWalletInput:')).toBeInTheDocument()
      expect(screen.getAllByText('braveWalletOutput:')).toHaveLength(2)
      expect(screen.getAllByText('braveWalletValue:')).toHaveLength(3)
      expect(screen.getAllByText('braveWalletAddress:')).toHaveLength(3)

      // Check specific values
      expect(screen.getAllByText('0')).toHaveLength(2)
      expect(screen.getByText('1')).toBeInTheDocument()
      expect(screen.getAllByText('10000000000000')).toHaveLength(3)

      // Check addresses
      expect(screen.getByText('bc1q4500000000000000000')).toBeInTheDocument()
    })
  })

  describe('Solana Transactions', () => {
    it('should render Solana transaction with send options', async () => {
      const store = createMockStoreWithAccounts()
      const renderOptions = renderComponentOptionsWithMockStore(store)

      render(
        <PendingTransactionDetails
          transactionInfo={mockSOLTXInstructions}
          instructions={getTypedSolanaTxInstructions(
            mockSOLTXInstructions.txDataUnion.solanaTxData,
          )}
        />,
        renderOptions,
      )

      await waitFor(() => {
        expect(
          screen.getByText('braveWalletSolanaMaxRetries'),
        ).toBeInTheDocument()
      })

      expect(screen.getByText('1')).toBeInTheDocument()
      expect(
        screen.getAllByText('"ComputeBudget111111111111111111111111111111"'),
      ).toHaveLength(2)
      expect(screen.getByText('[2,240,1,0,0]')).toBeInTheDocument()
      expect(screen.getByText('[3,1,0,0,0,0,0,0]')).toBeInTheDocument()
    })
  })

  describe('EVM & FIL Transactions', () => {
    it('should render EVM transaction with function type', () => {
      const evmTransaction: SerializableTransactionInfo = {
        ...mockTransactionInfo,
        txType: BraveWallet.TransactionType.ERC20Transfer,
        txParams: ['address', 'amount'],
        txArgs: ['0x123', '1000000000000000000'],
        txDataUnion: {
          ...mockTransactionInfo.txDataUnion,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559,
            chainId: '0x1',
            maxPriorityFeePerGas: '1000000000',
            maxFeePerGas: '2000000000',
            gasEstimation: undefined,
            baseData: {
              nonce: '0x1',
              gasPrice: '1000000000',
              gasLimit: '100000',
              to: '0x123',
              value: '0x0',
              signOnly: false,
              signedTransaction: undefined,
              data: [1, 2, 3, 4],
            },
          },
        },
      }

      const store = createMockStore({})
      const renderOptions = renderComponentOptionsWithMockStore(store)

      render(
        <PendingTransactionDetails transactionInfo={evmTransaction} />,
        renderOptions,
      )

      expect(
        screen.getByText('braveWalletTransactionDetailBoxFunction:'),
      ).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletTransactionTypeNameTokenTransfer'),
      ).toBeInTheDocument()
      expect(screen.getByText('address:')).toBeInTheDocument()
      expect(screen.getByText('0x123')).toBeInTheDocument()
      expect(screen.getByText('amount:')).toBeInTheDocument()
      expect(screen.getByText('1000000000000000000')).toBeInTheDocument()
    })
  })
})
