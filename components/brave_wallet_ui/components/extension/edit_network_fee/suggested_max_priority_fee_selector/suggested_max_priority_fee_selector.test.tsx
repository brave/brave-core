// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../../constants/types'

// Components
import {
  SuggestedMaxPriorityFeeSelector, //
} from './suggested_max_priority_fee_selector'

// Utils
import Amount from '../../../../utils/amount'
import {
  parseTransactionFeesWithoutPrices, //
} from '../../../../utils/tx-utils'

// Mocks
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'
import {
  mockSuggestedMaxPriorityFeeOptions,
  mockTransactionInfo,
} from '../../../../stories/mock-data/mock-transaction-info'
import { createMockStore } from '../../../../utils/test-utils'
import { default as BraveCoreThemeProvider } from '../../../../../common/BraveCoreThemeProvider'

// Mock the specific query hooks
jest.mock('../../../../common/slices/api.slice', () => {
  const originalModule = jest.requireActual(
    '../../../../common/slices/api.slice',
  )
  return {
    ...originalModule,
    useGetDefaultFiatCurrencyQuery: () => ({
      data: 'USD',
    }),
    useGetTokenSpotPricesQuery: () => ({
      data: [
        {
          coinType: 60, // ETH
          chainId: '0x1',
          address: '0x0000000000000000000000000000000000000000',
          price: '1600.92',
          percentageChange24h: '-0.69088',
          vsCurrency: 'USD',
          cacheStatus: BraveWallet.Gate3CacheStatus.kHit,
          source: BraveWallet.AssetPriceSource.kCoingecko,
        },
      ],
    }),
  }
})

describe('SuggestedMaxPriorityFeeSelector', () => {
  const baseFeePerGas = '0x59682f00' // 1500000000 wei (1.5 gwei)
  const transactionFees = parseTransactionFeesWithoutPrices(mockTransactionInfo)
  const suggestedFees = mockSuggestedMaxPriorityFeeOptions.map((option) =>
    new Amount(baseFeePerGas)
      .plus(option.fee)
      .times(transactionFees.gasLimit) // Wei-per-gas → Wei conversion
      .divideByDecimals(mockEthMainnet.decimals) // Wei → ETH conversion
      .format(4),
  )

  const renderComponent = () => {
    const store = createMockStore({})
    return render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SuggestedMaxPriorityFeeSelector
            transactionInfo={mockTransactionInfo}
            selectedNetwork={mockEthMainnet}
            baseFeePerGas={baseFeePerGas}
            suggestedMaxPriorityFee='average'
            suggestedMaxPriorityFeeOptions={mockSuggestedMaxPriorityFeeOptions}
            setSuggestedMaxPriorityFee={() => {}}
            onClickCustom={() => {}}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )
  }

  it('Should render suggested max priority fee options', async () => {
    renderComponent()

    await waitFor(() => {
      // Check if all fee options are rendered
      expect(screen.getByText('braveSwapSlow')).toBeInTheDocument()
      expect(screen.getByText('braveSwapAverage')).toBeInTheDocument()
      expect(screen.getByText('braveSwapFast')).toBeInTheDocument()

      // Check if durations are displayed
      expect(screen.getByText('28 min')).toBeInTheDocument()
      expect(screen.getByText('7 min')).toBeInTheDocument()
      expect(screen.getByText('1 min')).toBeInTheDocument()

      // Check if gas fees are displayed
      expect(
        screen.getByText(suggestedFees[0] + ' ' + mockEthMainnet.symbol),
      ).toBeInTheDocument()
      expect(
        screen.getByText(suggestedFees[1] + ' ' + mockEthMainnet.symbol),
      ).toBeInTheDocument()
      expect(
        screen.getByText(suggestedFees[2] + ' ' + mockEthMainnet.symbol),
      ).toBeInTheDocument()

      // Check if custom button is rendered
      expect(screen.getByText('braveWalletCustom')).toBeInTheDocument()

      // Check if Update button is rendered
      expect(screen.getByText('braveWalletUpdate')).toBeInTheDocument()
    })
  })
})
