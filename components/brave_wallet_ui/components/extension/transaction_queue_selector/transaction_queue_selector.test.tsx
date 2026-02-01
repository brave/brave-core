// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'

// Utils
import { default as BraveCoreThemeProvider } from '../../../../common/BraveCoreThemeProvider'

// Components
import { TransactionQueueSelector } from './transaction_queue_selector'

describe('AdvancedTransactionSettings', () => {
  it('should render the component', () => {
    const queueNextTransaction = jest.fn()
    const queuePreviousTransaction = jest.fn()
    const rejectAllTransactions = jest.fn()

    const { container } = render(
      <BraveCoreThemeProvider>
        <TransactionQueueSelector
          transactionsQueueLength={5}
          queueNextTransaction={queueNextTransaction}
          queuePreviousTransaction={queuePreviousTransaction}
          rejectAllTransactions={rejectAllTransactions}
        />
      </BraveCoreThemeProvider>,
    )

    // Check button
    const button = container.querySelector('leo-button')
    expect(button).toBeInTheDocument()
    expect(button).toHaveTextContent('braveWalletPendingTransactionsNumber')

    // Check menu items
    const menuItems = container.querySelectorAll('leo-menu-item')
    expect(menuItems).toHaveLength(3)
    expect(menuItems[0]).toHaveTextContent('braveWalletNextTransaction')
    expect(menuItems[1]).toHaveTextContent('braveWalletPreviousTransaction')
    expect(menuItems[2]).toHaveTextContent('braveWalletRejectTransactions')

    // Check Next transaction button
    const nextTransactionButton = menuItems[0] as HTMLElement
    nextTransactionButton.click()
    expect(queueNextTransaction).toHaveBeenCalled()

    // Check Previous transaction button
    const previousTransactionButton = menuItems[1] as HTMLElement
    previousTransactionButton.click()
    expect(queuePreviousTransaction).toHaveBeenCalled()

    // Check Reject transactions button
    const rejectTransactionsButton = menuItems[2] as HTMLElement
    rejectTransactionsButton.click()
    expect(rejectAllTransactions).toHaveBeenCalled()
  })
})
