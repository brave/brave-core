// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'

// Mock Data
import {
  mockCardanoAccount, //
} from '../../../stories/mock-data/mock-wallet-accounts'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { mockSolanaAccount } from '../../../common/constants/mocks'

// Utils
import {
  // eslint-disable-next-line import/no-named-default
  default as BraveCoreThemeProvider,
} from '../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../utils/test-utils'

// Components
import { SignPanel } from './index'

const signCardanoMessageData: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockCardanoAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ADA,
  chainId: BraveWallet.CARDANO_MAINNET,
  signData: {
    ethSignTypedData: undefined,
    solanaSignData: undefined,
    ethSiweData: undefined,
    ethStandardSignData: undefined,
    cardanoSignData: {
      message: 'Test Cardano Sign Data',
    },
  },
}

const signSolanaMessageData: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockSolanaAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.SOL,
  chainId: BraveWallet.SOLANA_MAINNET,
  signData: {
    ethSignTypedData: undefined,
    solanaSignData: {
      message: 'Test Solana Sign Data',
      messageBytes: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    },
    ethSiweData: undefined,
    ethStandardSignData: undefined,
    cardanoSignData: undefined,
  },
}

describe('SignTypedDataPanel', () => {
  it('Should show warning when cardano sign data is present', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SignPanel
            signMessageData={[signCardanoMessageData]}
            showWarning={true}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()

      // Network name
      expect(screen.getByText('Cardano Mainnet')).toBeInTheDocument()

      // Panel Title
      expect(
        screen.getByText('braveWalletSignTransactionTitle'),
      ).toBeInTheDocument()

      // Warning Title
      expect(
        screen.getByText('braveWalletSignWarningTitle'),
      ).toBeInTheDocument()

      // Warning Text
      expect(screen.getByText('braveWalletSignWarning')).toBeInTheDocument()

      // Buttons
      expect(screen.getByText('braveWalletButtonCancel')).toBeInTheDocument()
      expect(screen.getByText('braveWalletButtonContinue')).toBeInTheDocument()
    })
  })

  it('Should not show sign warning', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SignPanel
            signMessageData={[signSolanaMessageData]}
            showWarning={false}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )
    await waitFor(() => {
      expect(container).toBeVisible()

      // Network name
      expect(screen.getByText('Solana Mainnet Beta')).toBeInTheDocument()
    })
    // Panel Title
    expect(
      screen.getByText('braveWalletSignTransactionTitle'),
    ).toBeInTheDocument()

    // Warning Title should not be present
    expect(
      screen.queryByText('braveWalletSignWarningTitle'),
    ).not.toBeInTheDocument()

    // Buttons
    expect(screen.getByText('braveWalletButtonCancel')).toBeInTheDocument()
    expect(
      screen.getByText('braveWalletSignTransactionButton'),
    ).toBeInTheDocument()
  })
})
