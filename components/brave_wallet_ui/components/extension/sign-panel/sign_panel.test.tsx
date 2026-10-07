// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { fireEvent, render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'

// Mock Data
import {
  mockCardanoAccount,
  mockEthAccount, //
} from '../../../stories/mock-data/mock-wallet-accounts'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { mockSolanaAccount } from '../../../common/constants/mocks'

// Utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../utils/test-utils'

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

const signEthTypedDataMessage: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  signData: {
    ethSignTypedData: {
      addressParam: '',
      messageJson: 'Sign below to authenticate with CryptoKitties.',
      domainJson: '',
      typesJson: '',
      primaryType: 'Mail',
      chainId: '',
      domainHash: [],
      primaryHash: [],
      meta: undefined,
    },
    solanaSignData: undefined,
    ethSiweData: undefined,
    ethStandardSignData: undefined,
    cardanoSignData: undefined,
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
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[signCardanoMessageData]}
            showWarning={true}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()

      // Network name
      expect(screen.getByText('Cardano Mainnet')).toBeInTheDocument()

      // Panel Title
      expect(
        screen.getByText(S.BRAVE_WALLET_SIGN_TRANSACTION_TITLE),
      ).toBeInTheDocument()

      // Warning Title
      expect(
        screen.getByText(S.BRAVE_WALLET_SIGN_WARNING_TITLE),
      ).toBeInTheDocument()

      // Warning Text
      expect(screen.getByText(S.BRAVE_WALLET_SIGN_WARNING)).toBeInTheDocument()

      // Buttons
      expect(screen.getByText(S.BRAVE_WALLET_BUTTON_CANCEL)).toBeInTheDocument()
      expect(
        screen.getByText(S.BRAVE_WALLET_BUTTON_CONTINUE),
      ).toBeInTheDocument()
    })
  })

  it('Should show warning when eth sign typed data is present', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[signEthTypedDataMessage]}
            showWarning={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(container).toBeVisible()

      expect(
        screen.getByText(S.BRAVE_WALLET_SIGN_WARNING_TITLE),
      ).toBeInTheDocument()
      expect(screen.getByText(S.BRAVE_WALLET_SIGN_WARNING)).toBeInTheDocument()
      expect(
        screen.getByText(S.BRAVE_WALLET_BUTTON_CONTINUE),
      ).toBeInTheDocument()
    })
  })

  it('Should not show sign warning', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[signSolanaMessageData]}
            showWarning={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )
    await waitFor(() => {
      expect(container).toBeVisible()

      // Network name
      expect(screen.getByText('Solana Mainnet Beta')).toBeInTheDocument()
    })
    // Panel Title
    expect(
      screen.getByText(S.BRAVE_WALLET_SIGN_TRANSACTION_TITLE),
    ).toBeInTheDocument()

    // Warning Title should not be present
    expect(
      screen.queryByText(S.BRAVE_WALLET_SIGN_WARNING_TITLE),
    ).not.toBeInTheDocument()

    // Buttons
    expect(screen.getByText(S.BRAVE_WALLET_BUTTON_CANCEL)).toBeInTheDocument()
    expect(
      screen.getByText(S.BRAVE_WALLET_SIGN_TRANSACTION_BUTTON),
    ).toBeInTheDocument()
  })

  it('shows a formatted message when hidden characters are present', async () => {
    const store = createMockStore({})
    const message = 'Hello\0\n\u202E'
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[
              {
                ...signSolanaMessageData,
                signData: {
                  ...signSolanaMessageData.signData,
                  solanaSignData: {
                    message,
                    messageBytes: [1],
                  },
                },
              },
            ]}
            showWarning={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(
        screen.getByText(
          S.BRAVE_WALLET_SIGN_MESSAGE_UNEXPECTED_CHARACTERS_WARNING,
        ),
      ).toBeInTheDocument()
    })
    expect(container.textContent).toContain('Hello\\0\\n')
    expect(container.textContent).toContain('\\u202e')
    expect(container.textContent).not.toContain(message)

    fireEvent.click(screen.getByText(S.BRAVE_WALLET_VIEW_ENCODED_MESSAGE))

    expect(container.textContent).toContain(message)
    expect(container.textContent).not.toContain('Hello\\0')
    expect(
      screen.getByText(S.BRAVE_WALLET_VIEW_DECODED_MESSAGE),
    ).toBeInTheDocument()
  })

  it('does not warn for a plain message', async () => {
    const store = createMockStore({})
    render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[signSolanaMessageData]}
            showWarning={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(screen.getByText('Test Solana Sign Data')).toBeInTheDocument()
    })
    expect(
      screen.queryByText(S.BRAVE_WALLET_VIEW_ENCODED_MESSAGE),
    ).not.toBeInTheDocument()
    expect(
      screen.queryByText(
        S.BRAVE_WALLET_SIGN_MESSAGE_UNEXPECTED_CHARACTERS_WARNING,
      ),
    ).not.toBeInTheDocument()
  })

  it('warns for newlines in an eth standard sign message', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[
              {
                id: 1,
                accountId: mockEthAccount.accountId,
                originInfo: mockOriginInfo,
                coin: BraveWallet.CoinType.ETH,
                chainId: BraveWallet.MAINNET_CHAIN_ID,
                signData: {
                  ethSignTypedData: undefined,
                  solanaSignData: undefined,
                  ethSiweData: undefined,
                  cardanoSignData: undefined,
                  ethStandardSignData: {
                    message: 'Main Message\nHidden payload',
                  },
                },
              },
            ]}
            showWarning={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(
        screen.getByText(
          S.BRAVE_WALLET_SIGN_MESSAGE_UNEXPECTED_CHARACTERS_WARNING,
        ),
      ).toBeInTheDocument()
    })
    expect(container.textContent).toContain('Main Message\\n')
    expect(container.textContent).toContain('Hidden payload')
    expect(
      screen.queryByText(S.BRAVE_WALLET_SIGN_WARNING_TITLE),
    ).not.toBeInTheDocument()
  })

  it('warns for hidden characters in typed data after the risk step', async () => {
    const store = createMockStore({})
    const { container } = render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignPanel
            signMessageData={[
              {
                ...signEthTypedDataMessage,
                signData: {
                  ...signEthTypedDataMessage.signData,
                  ethSignTypedData: {
                    addressParam: '',
                    messageJson: '{"contents":"Sign into \u202E EVIL"}',
                    domainJson: '{"name":"domain\\u0000"}',
                    typesJson: '',
                    primaryType: 'Mail',
                    chainId: '',
                    domainHash: [],
                    primaryHash: [],
                    meta: undefined,
                  },
                },
              },
            ]}
            showWarning={false}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    await waitFor(() => {
      expect(
        screen.getByText(S.BRAVE_WALLET_SIGN_WARNING_TITLE),
      ).toBeInTheDocument()
    })
    expect(
      screen.queryByText(
        S.BRAVE_WALLET_SIGN_MESSAGE_UNEXPECTED_CHARACTERS_WARNING,
      ),
    ).not.toBeInTheDocument()

    fireEvent.click(screen.getByText(S.BRAVE_WALLET_BUTTON_CONTINUE))

    expect(
      screen.getByText(
        S.BRAVE_WALLET_SIGN_MESSAGE_UNEXPECTED_CHARACTERS_WARNING,
      ),
    ).toBeInTheDocument()
    expect(container.textContent).toContain('domain\\0')
    expect(container.textContent).toContain('\\u202e')
  })
})
