// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'

// Mock Data
import {
  mockCardanoAccount, //
} from '../../../stories/mock-data/mock-wallet-accounts'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import {
  mockEthAccount,
  mockSolanaAccount,
} from '../../../common/constants/mocks'

// Utils
import BraveCoreThemeProvider from '../../../../common/BraveCoreThemeProvider'
import { createMockStore } from '../../../utils/test-utils'

// Components
import { SignPanel } from './index'

function makeEthSignTypedData(
  overrides: Partial<BraveWallet.EthSignTypedData>,
): BraveWallet.EthSignTypedData {
  return {
    addressParam: '0x',
    chainId: '1',
    domainHash: [],
    domainJson: '{}',
    messageJson: '{}',
    meta: undefined,
    primaryHash: [],
    primaryType: 'Person',
    typesJson: '{}',
    ...overrides,
  }
}

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

const signEthPermitTypedDataRequest: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  signData: {
    ethSignTypedData: makeEthSignTypedData({
      primaryType: 'Permit',
      typesJson: '{}',
    }),
    solanaSignData: undefined,
    ethSiweData: undefined,
    ethStandardSignData: undefined,
    cardanoSignData: undefined,
  },
}

const signEthTransferWithAuthorizationRequest: BraveWallet.SignMessageRequest =
  {
    id: 0,
    accountId: mockEthAccount.accountId,
    originInfo: mockOriginInfo,
    coin: BraveWallet.CoinType.ETH,
    chainId: BraveWallet.MAINNET_CHAIN_ID,
    signData: {
      ethSignTypedData: makeEthSignTypedData({
        primaryType: 'TransferWithAuthorization',
        typesJson: '{}',
      }),
      solanaSignData: undefined,
      ethSiweData: undefined,
      ethStandardSignData: undefined,
      cardanoSignData: undefined,
    },
  }

const signEthNestedPermit2Request: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  signData: {
    ethSignTypedData: makeEthSignTypedData({
      primaryType: 'Nested',
      typesJson: JSON.stringify({
        Nested: [{ name: 'permit', type: 'PermitSingle' }],
        PermitSingle: [
          { name: 'details', type: 'PermitDetails' },
          { name: 'spender', type: 'address' },
          { name: 'sigDeadline', type: 'uint256' },
        ],
      }),
    }),
    solanaSignData: undefined,
    ethSiweData: undefined,
    ethStandardSignData: undefined,
    cardanoSignData: undefined,
  },
}

const signEthPermitShapeTypedDataRequest: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  signData: {
    ethSignTypedData: makeEthSignTypedData({
      primaryType: 'CustomPermit',
      typesJson: JSON.stringify({
        CustomPermit: [
          { name: 'owner', type: 'address' },
          { name: 'spender', type: 'address' },
          { name: 'value', type: 'uint256' },
        ],
      }),
    }),
    solanaSignData: undefined,
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

  it('Should show sign risk warning for EIP-712 Permit primary type', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SignPanel
            signMessageData={[signEthPermitTypedDataRequest]}
            showWarning={false}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()
      expect(screen.getByText('Ethereum Mainnet')).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletSignWarningTitle'),
      ).toBeInTheDocument()
      expect(screen.getByText('braveWalletSignWarning')).toBeInTheDocument()
      expect(screen.getByText('braveWalletButtonContinue')).toBeInTheDocument()
    })
  })

  it('Should show sign risk warning for EIP-3009 TransferWithAuthorization', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SignPanel
            signMessageData={[signEthTransferWithAuthorizationRequest]}
            showWarning={false}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()
      expect(screen.getByText('Ethereum Mainnet')).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletSignWarningTitle'),
      ).toBeInTheDocument()
      expect(screen.getByText('braveWalletSignWarning')).toBeInTheDocument()
      expect(screen.getByText('braveWalletButtonContinue')).toBeInTheDocument()
    })
  })

  it('Should show sign risk warning for permit-shaped EIP-712 message', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SignPanel
            signMessageData={[signEthPermitShapeTypedDataRequest]}
            showWarning={false}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()
      expect(screen.getByText('Ethereum Mainnet')).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletSignWarningTitle'),
      ).toBeInTheDocument()
      expect(screen.getByText('braveWalletSignWarning')).toBeInTheDocument()
      expect(screen.getByText('braveWalletButtonContinue')).toBeInTheDocument()
    })
  })

  it('Should show sign risk warning when permit2 is nested under benign primary type', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <BraveCoreThemeProvider>
          <SignPanel
            signMessageData={[signEthNestedPermit2Request]}
            showWarning={false}
          />
        </BraveCoreThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()
      expect(screen.getByText('Ethereum Mainnet')).toBeInTheDocument()
      expect(
        screen.getByText('braveWalletSignWarningTitle'),
      ).toBeInTheDocument()
      expect(screen.getByText('braveWalletSignWarning')).toBeInTheDocument()
      expect(screen.getByText('braveWalletButtonContinue')).toBeInTheDocument()
    })
  })
})
