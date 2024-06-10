// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'

// components
import {
  WalletPanelTestWrapper //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { PendingTransactionPanel } from './pending_transaction_panel'

// mocks
import {
  mockTransactionInfo //
} from '../../../stories/mock-data/mock-transaction-info'

test('should display transaction origin after loading data', async () => {
  render(
    <WalletPanelTestWrapper>
      <PendingTransactionPanel
        selectedPendingTransaction={mockTransactionInfo}
      />
    </WalletPanelTestWrapper>
  )

  // loading skeleton should appear first
  expect(
    screen.getByTestId('confirm-transaction-panel-loading-skeleton')
  ).toBeVisible()

  // wait for loading to finish
  await waitFor(() =>
    expect(
      screen.queryByTestId('confirm-transaction-panel-loading-skeleton')
    ).not.toBeInTheDocument()
  )

  // then the transaction origin should appear
  expect(screen.getByText('uniswap.org')).toBeVisible()
  expect(screen.getByText('uniswap.org')).toHaveTextContent('uniswap.org')
})
