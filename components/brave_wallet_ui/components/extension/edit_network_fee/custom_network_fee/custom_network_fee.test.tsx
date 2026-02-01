// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { fireEvent, render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Utils
import { default as BraveCoreThemeProvider } from '../../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../../utils/test-utils'

// Components
import { CustomNetworkFee } from './custom_network_fee'

// Mock data
import {
  mockTransactionInfo, //
} from '../../../../stories/mock-data/mock-transaction-info'
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'

describe('CustomNetworkFee', () => {
  const mockBaseFeePerGas = '20000000000' // 20 Gwei in Wei

  const mockOnUpdateCustomNetworkFee = jest.fn()
  const mockOnBack = jest.fn()
  const mockOnClose = jest.fn()

  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('should render custom network fee component correctly', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <CustomNetworkFee
            transactionInfo={mockTransactionInfo}
            selectedNetwork={mockEthMainnet}
            baseFeePerGas={mockBaseFeePerGas}
            onUpdateCustomNetworkFee={mockOnUpdateCustomNetworkFee}
            onBack={mockOnBack}
            onClose={mockOnClose}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()

      // Check for description text
      expect(
        screen.getByText('braveWalletEditGasDescription'),
      ).toBeInTheDocument()

      // Check for gas limit input
      expect(screen.getByText('braveWalletEditGasLimit')).toBeInTheDocument()

      // Check for base fee display (for EIP1559 transactions)
      expect(screen.getByText('braveWalletEditGasBaseFee')).toBeInTheDocument()

      // Check for gas tip limit (for EIP1559 transactions)
      expect(screen.getByText('braveWalletGasTipLimit')).toBeInTheDocument()

      // Check for gas price limit (for EIP1559 transactions)
      expect(screen.getByText('braveWalletGasPriceLimit')).toBeInTheDocument()

      // Check for update button
      expect(screen.getByText('braveWalletUpdate')).toBeInTheDocument()

      // Check for use default button (for EIP1559 transactions)
      expect(screen.getByText('braveWalletUseDefault')).toBeInTheDocument()

      // Check for gas limit error
      const gasLimitInput = screen.getByTestId('gas-limit-input')
      fireEvent.change(gasLimitInput, { target: { value: '0' } })
      expect(screen.getByText('braveWalletEditGasLimitError')).toBeVisible()

      // Check for gas price limit error
      const gasPriceLimitInput = screen.getByTestId('gas-price-limit-input')
      fireEvent.change(gasPriceLimitInput, { target: { value: '0' } })
      expect(
        screen.getByText('braveWalletGasFeeLimitLowerThanBaseFeeWarning'),
      ).toBeVisible()
    })
  })

  it('should render non-EIP1559 transaction correctly', async () => {
    const nonEip1559TransactionInfo = {
      ...mockTransactionInfo,
      txDataUnion: {
        ethTxData1559: undefined,
        ethTxData: {
          nonce: '0x1',
          gasPrice: '0x59682f00',
          gasLimit: '0x5208',
          to: '0x0987654321098765432109876543210987654321',
          value: '0x0',
          data: [],
          signOnly: false,
          signedTransaction: undefined,
        },
        solanaTxData: undefined,
        filTxData: undefined,
        btcTxData: undefined,
        zecTxData: undefined,
      },
    }

    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <CustomNetworkFee
            transactionInfo={nonEip1559TransactionInfo}
            selectedNetwork={mockEthMainnet}
            baseFeePerGas={mockBaseFeePerGas}
            onUpdateCustomNetworkFee={mockOnUpdateCustomNetworkFee}
            onBack={mockOnBack}
            onClose={mockOnClose}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()

      // Check for gas price input (for non-EIP1559 transactions)
      expect(screen.getByText('braveWalletGasPrice')).toBeInTheDocument()

      // Should not show EIP1559 specific fields
      expect(
        screen.queryByText('braveWalletEditGasBaseFee'),
      ).not.toBeInTheDocument()
      expect(
        screen.queryByText('braveWalletGasTipLimit'),
      ).not.toBeInTheDocument()
      expect(
        screen.queryByText('braveWalletGasPriceLimit'),
      ).not.toBeInTheDocument()
      expect(
        screen.queryByText('braveWalletUseDefault'),
      ).not.toBeInTheDocument()

      // Check for gas price error
      const gasPriceInput = screen.getByTestId('gas-price-input')
      fireEvent.change(gasPriceInput, { target: { value: '0' } })
      expect(
        screen.getByText('braveWalletEditGasZeroGasPriceWarning'),
      ).toBeVisible()
    })
  })
})
