// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { Provider } from 'react-redux'

// Utils
import { default as BraveCoreThemeProvider } from '../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../utils/test-utils'

// Components
import { EditSpendLimit } from './edit_spend_limit'

describe('EditSpendLimit', () => {
  const renderComponent = () => {
    const store = createMockStore({})
    return render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <EditSpendLimit
            onCancel={jest.fn()}
            onSave={jest.fn()}
            proposedAllowance='100'
            symbol='ETH'
            approvalTarget='Uniswap V3'
            isApprovalUnlimited={false}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )
  }

  it('should render the component', () => {
    renderComponent()

    expect(
      screen.getByText('braveWalletEditPermissionsDescription'),
    ).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletProposedSpendLimit'),
    ).toBeInTheDocument()
    expect(screen.getByText('100 ETH')).toBeInTheDocument()
    expect(screen.getByText('braveWalletCustomSpendLimit')).toBeInTheDocument()
    expect(screen.getByText('braveWalletButtonCancel')).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletAccountSettingsSave'),
    ).toBeInTheDocument()
  })
})
