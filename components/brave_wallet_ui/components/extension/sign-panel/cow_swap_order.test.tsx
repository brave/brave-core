// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor } from '@testing-library/react'
import { Provider } from 'react-redux'

// Types
import { BraveWallet } from '../../../constants/types'

// Mock Data
import { mockEthAccount } from '../../../stories/mock-data/mock-wallet-accounts'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { mockUSDCoin } from '../../../stories/mock-data/mock-asset-options'
import {
  mockEthMainnet,
  mockNetworks,
} from '../../../stories/mock-data/mock-networks'

// Utils
import {
  createMockStore,
  WalletTestThemeProvider,
} from '../../../utils/test-utils'

// Components
import { SignCowSwapOrder } from './cow_swap_order'

const kUsdcAddress = '0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48'
const kWethAddress = '0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2'

const mockBaseNetwork: BraveWallet.NetworkInfo = {
  ...mockEthMainnet,
  chainId: BraveWallet.BASE_MAINNET_CHAIN_ID,
  chainName: 'Base',
}

const mainnetUsdc: BraveWallet.BlockchainToken = {
  ...mockUSDCoin,
  contractAddress: kUsdcAddress,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  decimals: 6,
  symbol: 'USDC',
}

// Watched on Base with the mainnet USDC address and attacker-supplied decimals.
const baseShadowUsdc: BraveWallet.BlockchainToken = {
  ...mainnetUsdc,
  chainId: BraveWallet.BASE_MAINNET_CHAIN_ID,
  contractAddress: kUsdcAddress.toLowerCase(),
  decimals: 18,
}

const mainnetWeth: BraveWallet.BlockchainToken = {
  ...mockUSDCoin,
  contractAddress: kWethAddress,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  decimals: 18,
  symbol: 'WETH',
  name: 'Wrapped Ether',
  coingeckoId: 'weth',
}

const cowSwapSignRequest: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  signData: {
    ethStandardSignData: undefined,
    ethSiweData: undefined,
    solanaSignData: undefined,
    cardanoSignData: undefined,
    ethSignTypedData: {
      addressParam: mockEthAccount.address,
      domainJson: '',
      domainHash: [],
      messageJson: '',
      typesJson: '',
      primaryType: 'Order',
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      primaryHash: [],
      meta: {
        cowSwapOrder: {
          buyAmount: '10000000000000000',
          buyToken: kWethAddress,
          deadline: '1798761600',
          receiver: '0x000000000000000000000000000000000000dEaD',
          sellAmount: '100000000',
          sellToken: kUsdcAddress,
        },
      },
    },
  },
}

describe('SignCowSwapOrder', () => {
  it('uses the signed chain when the same address is watched on another chain', async () => {
    const store = createMockStore(
      {},
      {
        networks: [...mockNetworks, mockBaseNetwork],
        // User assets are listed before registry tokens.
        userAssets: [baseShadowUsdc],
        blockchainTokens: [mainnetUsdc, mainnetWeth],
      },
    )

    render(
      <Provider store={store}>
        <WalletTestThemeProvider>
          <SignCowSwapOrder
            data={cowSwapSignRequest}
            queueLength={1}
            queueNumber={1}
            isDisabled={false}
            onQueueNextSignMessage={() => {}}
            onSignIn={() => {}}
            onCancel={() => {}}
          />
        </WalletTestThemeProvider>
      </Provider>,
    )

    // 100000000 base units is 100 USDC at 6 decimals, and 0.0000000001 at 18.
    await waitFor(() => {
      expect(screen.getByText('100 USDC')).toBeInTheDocument()
    })
    expect(screen.queryByText('0.0000000001 USDC')).not.toBeInTheDocument()
  })
})
