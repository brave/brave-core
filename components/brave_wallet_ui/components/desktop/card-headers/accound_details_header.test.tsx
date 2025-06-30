// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Utils
import BraveCoreThemeProvider from '../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../utils/test-utils'
import { reduceAddress } from '../../../utils/reduce-address'

// Components
import { AccountDetailsHeader } from './account-details-header'

// Mocks
import {
  mockAccount,
  mockTokenBalanceRegistry, //
} from '../../../common/constants/mocks'

interface RenderComponentProps {
  isAndroid: boolean
  isPanel: boolean
}

describe('AccountDetailsHeader', () => {
  const renderComponent = (props: RenderComponentProps) => {
    const store = createMockStore({
      uiStateOverride: {
        isAndroid: props.isAndroid,
        isPanel: props.isPanel,
      },
    })
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <AccountDetailsHeader
            account={mockAccount}
            onClickMenuOption={() => {}}
            tokenBalancesRegistry={mockTokenBalanceRegistry}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )
    return { container }
  }

  it('renders account details header desktop', async () => {
    const { container } = renderComponent({
      isAndroid: false,
      isPanel: false,
    })
    // Wait for all async operations to complete
    await waitFor(() => {
      // Test Mock Data
      expect(container).toBeInTheDocument()
      expect(screen.getByText(mockAccount.name)).toBeInTheDocument()
      expect(
        screen.getByText(reduceAddress(mockAccount.address)),
      ).toBeInTheDocument()

      // Should be visible on desktop
      expect(
        container.querySelector('[data-key="account-balance-column"]'),
      ).toBeInTheDocument()

      // Test Styles
      expect(
        container.querySelector('[data-key="account-details-header"]'),
      ).toHaveStyle({
        padding: '24px 0px',
      })
    })
  })

  it('renders account details header panel', async () => {
    const { container } = renderComponent({
      isAndroid: false,
      isPanel: true,
    })

    await waitFor(() => {
      // Should not be visible on panel
      expect(
        container.querySelector('[data-key="account-balance-column"]'),
      ).not.toBeInTheDocument()

      // Test Styles
      expect(
        container.querySelector('[data-key="account-details-header"]'),
      ).toHaveStyle({
        padding: '16px',
      })
    })
  })

  it('renders account details header android', async () => {
    const { container } = renderComponent({
      isAndroid: true,
      isPanel: false,
    })

    await waitFor(() => {
      // Should not be visible on android
      expect(
        container.querySelector('[data-key="account-balance-column"]'),
      ).not.toBeInTheDocument()

      // Test Styles
      expect(
        container.querySelector('[data-key="account-details-header"]'),
      ).toHaveStyle({
        padding: '16px',
      })
    })
  })
})
