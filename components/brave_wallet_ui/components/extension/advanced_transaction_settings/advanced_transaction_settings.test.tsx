// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'

// Utils
import { WalletTestThemeProvider } from '../../../utils/test-utils'

// Components
import { AdvancedTransactionSettings } from './advanced_transaction_settings'

describe('AdvancedTransactionSettings', () => {
  it('should render the component', () => {
    const { container } = render(
      <WalletTestThemeProvider>
        <AdvancedTransactionSettings
          nonce='5'
          onCancel={jest.fn()}
          onSave={jest.fn()}
        />
      </WalletTestThemeProvider>,
    )

    // Check locale
    expect(container).toBeInTheDocument()
    expect(container).toHaveTextContent(S.BRAVE_WALLET_EDIT_NONCE)
    expect(container).toHaveTextContent(S.BRAVE_WALLET_ACCOUNT_SETTINGS_SAVE)
    expect(container).toHaveTextContent(S.BRAVE_WALLET_BUTTON_CANCEL)

    // Check input
    const input = container.querySelector('input')
    expect(input).toBeInTheDocument()
    expect(input).toHaveAttribute('type', 'number')
    expect(input).toHaveAttribute('value', '5')

    // Check buttons
    const buttons = container.querySelectorAll('leo-button')

    // Check Cancel button
    const cancelButton = buttons[0]
    expect(cancelButton).toBeInTheDocument()
    expect(cancelButton).toHaveTextContent(S.BRAVE_WALLET_BUTTON_CANCEL)

    // Check Save button
    const saveButton = buttons[1]
    expect(saveButton).toBeInTheDocument()
    expect(saveButton).toHaveTextContent(S.BRAVE_WALLET_ACCOUNT_SETTINGS_SAVE)
  })
})
