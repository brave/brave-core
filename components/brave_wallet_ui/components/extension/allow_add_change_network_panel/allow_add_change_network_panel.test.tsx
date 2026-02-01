// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Utils
import { default as BraveCoreThemeProvider } from '../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../utils/test-utils'

// Components
import { AllowAddChangeNetworkPanel } from './allow_add_change_network_panel'

// Mocks
import {
  mockSwitchChainRequest,
  mockAddChainRequest,
} from '../../../stories/mock-data/mock-eth-requests'

describe('AllowAddChangeNetworkPanel', () => {
  const renderComponent = (props: any) => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <AllowAddChangeNetworkPanel {...props} />
        </BraveCoreThemeProvider>
      </Provider>,
    )
    return { container }
  }

  it('should render add network panel correctly', async () => {
    const { container } = renderComponent({
      addChainRequest: mockAddChainRequest,
    })

    await waitFor(() => {
      // Check if the component renders
      expect(container).toBeInTheDocument()

      // Check if the header is displayed
      expect(screen.getByText('braveWalletAddNetwork')).toBeInTheDocument()

      // Check if the title is displayed for add network
      expect(
        screen.getByText('braveWalletAllowAddNetworkTitle'),
      ).toBeInTheDocument()

      // Check if the description is displayed
      expect(
        screen.getByText('braveWalletAllowAddNetworkDescription'),
      ).toBeInTheDocument()

      // Check if the learn more button is displayed
      expect(
        screen.getByText('braveWalletAllowAddNetworkLearnMoreButton'),
      ).toBeInTheDocument()

      // Check if the network name is displayed
      expect(
        screen.getAllByText(mockAddChainRequest.networkInfo.chainName),
      ).toHaveLength(3)

      // Check if the network URL is displayed
      expect(
        screen.getAllByText(
          mockAddChainRequest.networkInfo.rpcEndpoints[0].url,
        ),
      ).toHaveLength(2)

      // Check if the details button is displayed
      expect(
        screen.getByText('braveWalletAllowAddNetworkDetailsButton'),
      ).toBeInTheDocument()

      // Check if the action buttons are displayed
      expect(screen.getByText('braveWalletButtonCancel')).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletAllowAddNetworkButton'),
      ).toBeInTheDocument()
    })
  })

  it('should render switch network panel correctly', async () => {
    const { container } = renderComponent({
      switchChainRequest: mockSwitchChainRequest,
    })

    await waitFor(() => {
      // Check if the component renders
      expect(container).toBeInTheDocument()

      // Check if the header is displayed
      expect(
        screen.getAllByText('braveWalletAllowChangeNetworkButton'),
      ).toHaveLength(2)

      // Check if the title is displayed for switch network
      expect(
        screen.getByText('braveWalletAllowChangeNetworkTitle'),
      ).toBeInTheDocument()

      // Check if the description is displayed
      expect(
        screen.getByText('braveWalletAllowChangeNetworkDescription'),
      ).toBeInTheDocument()

      // Check if the "From" network info is displayed
      expect(screen.getByText('braveWalletFrom:')).toBeInTheDocument()

      // Check if the "To" network info is displayed
      expect(screen.getByText('braveWalletSwapTo:')).toBeInTheDocument()

      // Check if the details button is displayed
      expect(screen.getByText('braveWalletDetails')).toBeInTheDocument()

      // Check if the cancel button is displayed
      expect(screen.getByText('braveWalletButtonCancel')).toBeInTheDocument()
    })
  })

  it('should display origin information correctly', async () => {
    const { container } = renderComponent({
      addChainRequest: mockAddChainRequest,
    })

    await waitFor(() => {
      // Check if the origin is displayed
      expect(container).toHaveTextContent(
        mockAddChainRequest.originInfo.eTldPlusOne,
      )

      // Check if the favicon is displayed
      const favIcon = container.querySelector('img[src*="chrome://favicon2"]')
      expect(favIcon).toBeInTheDocument()
    })
  })
})
