// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Components
import {
  SuggestedMaxPriorityFeeSelector, //
} from './suggested_max_priority_fee_selector'

// Mocks
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'
import {
  mockSuggestedMaxPriorityFeeOptions,
  mockTransactionInfo,
} from '../../../../stories/mock-data/mock-transaction-info'
import { createMockStore } from '../../../../utils/test-utils'
import {
  // eslint-disable-next-line import/no-named-default
  default as BraveCoreThemeProvider,
} from '../../../../../common/BraveCoreThemeProvider'

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
      data: {
        'ethereum': {
          usd: 1600.92,
          usd_24h_change: -0.69088,
          usd_24h_vol: 6076168182,
          usd_market_cap: 223895622340,
        },
      },
    }),
  }
})

describe('SuggestedMaxPriorityFeeSelector', () => {
  const renderComponent = () => {
    const store = createMockStore({})
    return render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SuggestedMaxPriorityFeeSelector
            transactionInfo={mockTransactionInfo}
            selectedNetwork={mockEthMainnet}
            baseFeePerGas={'0x59682f00'} // 1500000000 wei (1.5 gwei)
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
      expect(screen.getByText('0.003048 ETH')).toBeInTheDocument()
      expect(screen.getByText('0.01171 ETH')).toBeInTheDocument()
      expect(screen.getByText('0.02664 ETH')).toBeInTheDocument()

      // Check if custom button is rendered
      expect(screen.getByText('braveWalletCustom')).toBeInTheDocument()

      // Check if Update button is rendered
      expect(screen.getByText('braveWalletUpdate')).toBeInTheDocument()
    })
  })
})
