// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'

// Utils
import { default as BraveCoreThemeProvider } from '../../../../common/BraveCoreThemeProvider'

// Components
import { AdvancedTransactionSettings } from './advanced_transaction_settings'

describe('AdvancedTransactionSettings', () => {
  it('should render the component', () => {
    const { container } = render(
      <BraveCoreThemeProvider>
        <AdvancedTransactionSettings
          nonce='5'
          onCancel={jest.fn()}
          onSave={jest.fn()}
        />
      </BraveCoreThemeProvider>,
    )

    // Check locale
    expect(container).toBeInTheDocument()
    expect(container).toHaveTextContent('braveWalletEditNonce')
    expect(container).toHaveTextContent('braveWalletAccountSettingsSave')
    expect(container).toHaveTextContent('braveWalletButtonCancel')

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
    expect(cancelButton).toHaveTextContent('braveWalletButtonCancel')

    // Check Save button
    const saveButton = buttons[1]
    expect(saveButton).toBeInTheDocument()
    expect(saveButton).toHaveTextContent('braveWalletAccountSettingsSave')
  })
})
