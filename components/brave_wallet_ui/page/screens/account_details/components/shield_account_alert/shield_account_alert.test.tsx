// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, act, waitFor } from '@testing-library/react'

// Utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../../../utils/test-utils'

// Components
import { ShieldAccountAlert } from './shield_account_alert'

// Mocks
import { mockZecAccount } from '../../../../../common/constants/mocks'
import { Provider } from 'react-redux'

describe('ShieldAccountAlert', () => {
  it('should render', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <ShieldAccountAlert account={mockZecAccount} />
        </WalletTestThemeProvider>
      </Provider>,
    )

    // Check if the shield account alert is rendered
    expect(container).toBeInTheDocument()
    expect(container).toHaveTextContent(S.BRAVE_WALLET_SHIELD_ACCOUNT)
    expect(container).toHaveTextContent(
      S.BRAVE_WALLET_SHIELD_ACCOUNT_ALERT_DESCRIPTION,
    )

    // Check if the shield account alert button is rendered
    const shieldAccountAlertButton: any = document.querySelector('leo-button')
    expect(shieldAccountAlertButton).toBeInTheDocument()
    expect(shieldAccountAlertButton?.textContent).toBe(
      S.BRAVE_WALLET_SHIELD_ACCOUNT,
    )

    // Check if the shield account alert button is clickable and opens the modal
    act(() => {
      shieldAccountAlertButton?.shadowRoot?.querySelector('button').click()
    })

    await waitFor(() => {
      expect(container).toHaveTextContent(
        S.BRAVE_WALLET_SWITCH_TO_SHIELDED_ACCOUNT,
      )
      expect(container).toHaveTextContent(
        S.BRAVE_WALLET_ACCOUNT_NOT_SHIELDED_DESCRIPTION,
      )
      expect(container).toHaveTextContent(
        S.BRAVE_WALLET_ACCOUNT_SHIELDED_DESCRIPTION,
      )
      expect(container).toHaveTextContent(
        S.BRAVE_WALLET_ADVANCED_TRANSACTION_SETTINGS,
      )
      expect(container).toHaveTextContent(S.BRAVE_WALLET_SHIELD_ACCOUNT)
    })
  })
})
