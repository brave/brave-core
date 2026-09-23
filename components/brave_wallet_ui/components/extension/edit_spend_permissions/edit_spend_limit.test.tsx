// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { Provider } from 'react-redux'

// Utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../utils/test-utils'

// Components
import { EditSpendLimit } from './edit_spend_limit'

describe('EditSpendLimit', () => {
  const renderComponent = () => {
    const store = createMockStore({})
    return render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <EditSpendLimit
            onCancel={jest.fn()}
            onSave={jest.fn()}
            proposedAllowance='100'
            symbol='ETH'
            approvalTarget='Uniswap V3'
            isApprovalUnlimited={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )
  }

  it('should render the component', () => {
    renderComponent()

    expect(
      screen.getByText(S.BRAVE_WALLET_EDIT_PERMISSIONS_DESCRIPTION),
    ).toBeInTheDocument()
    expect(
      screen.getByText(S.BRAVE_WALLET_PROPOSED_SPEND_LIMIT),
    ).toBeInTheDocument()
    expect(screen.getByText('100 ETH')).toBeInTheDocument()
    expect(
      screen.getByText(S.BRAVE_WALLET_CUSTOM_SPEND_LIMIT),
    ).toBeInTheDocument()
    expect(screen.getByText(S.BRAVE_WALLET_BUTTON_CANCEL)).toBeInTheDocument()
    expect(
      screen.getByText(S.BRAVE_WALLET_ACCOUNT_SETTINGS_SAVE),
    ).toBeInTheDocument()
  })
})
