// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// asset
import { USDCIconUrl } from './asset-icons'

// types
import { BraveWallet, WalletState } from '../../constants/types'
import { AllNetworksOptionDefault } from '../../options/network-filter-options'
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import { AllAccountsOptionUniqueKey } from '../../options/account-filter-options'
import { AccountsGroupByOption } from '../../options/group-assets-by-options'

// mocks
import { mockMoonCatNFT, mockErc20TokensList } from './mock-asset-options'
import { networkEntityAdapter } from '../../common/slices/entities/network.entity'

export const mockWalletState: WalletState = {
  activeOrigin: {
    originSpec: 'https://app.uniswap.org',
    eTldPlusOne: 'uniswap.org'
  },
  addUserAssetError: false,
  fullTokenList: mockErc20TokensList,
  hasInitialized: true,
  isBitcoinEnabled: false,
  isZCashEnabled: false,
  isAnkrBalancesFeatureEnabled: false,
  allowedNewWalletAccountTypeNetworkIds: [
    networkEntityAdapter.selectId({
      chainId: BraveWallet.FILECOIN_MAINNET,
      coin: BraveWallet.CoinType.FIL
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.FILECOIN_TESTNET,
      coin: BraveWallet.CoinType.FIL
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.BITCOIN_MAINNET,
      coin: BraveWallet.CoinType.BTC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.BITCOIN_TESTNET,
      coin: BraveWallet.CoinType.BTC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.Z_CASH_MAINNET,
      coin: BraveWallet.CoinType.ZEC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.Z_CASH_TESTNET,
      coin: BraveWallet.CoinType.ZEC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.SOLANA_MAINNET,
      coin: BraveWallet.CoinType.SOL
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH
    })
  ],
  isWalletCreated: false,
  isWalletLocked: false,
  userVisibleTokensInfo: [
    {
      coingeckoId: '',
      contractAddress: '',
      decimals: 18,
      isErc20: false,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      isSpam: false,
      logo: 'chrome://erc-token-images/',
      name: 'Ethereum',
      symbol: 'ETH',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID
    },
    {
      coingeckoId: 'usd-coin',
      contractAddress: '0x07865c6E87B9F70255377e024ace6630C1Eaa37F',
      decimals: 6,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      isSpam: false,
      logo: USDCIconUrl,
      name: 'USD Coin',
      symbol: 'USDC',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID
    },
    {
      coingeckoId: '',
      contractAddress: '',
      decimals: 18,
      isErc20: false,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      isSpam: false,
      logo: 'chrome://erc-token-images/',
      name: 'Ethereum',
      symbol: 'ETH',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.GOERLI_CHAIN_ID
    },
    {
      coingeckoId: 'usd-coin',
      contractAddress: '0x07865c6E87B9F70255377e024ace6630C1Eaa37F',
      decimals: 6,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      isSpam: false,
      logo: USDCIconUrl,
      name: 'USD Coin',
      symbol: 'USDC',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.GOERLI_CHAIN_ID
    },
    mockMoonCatNFT
  ],
  selectedNetworkFilter: AllNetworksOptionDefault,
  selectedAssetFilter: HighToLowAssetsFilterOption.id,
  selectedAccountFilter: AllAccountsOptionUniqueKey,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: false,
  isNftPinningFeatureEnabled: false,
  hidePortfolioBalances: false,
  hidePortfolioGraph: false,
  removedFungibleTokenIds: [],
  removedNonFungibleTokenIds: [],
  removedNonFungibleTokens: [],
  deletedNonFungibleTokenIds: [],
  deletedNonFungibleTokens: [],
  hidePortfolioNFTsTab: false,
  filteredOutPortfolioNetworkKeys: [],
  filteredOutPortfolioAccountIds: [],
  hidePortfolioSmallBalances: false,
  selectedGroupAssetsByItem: AccountsGroupByOption.id,
  showNetworkLogoOnNfts: false,
  isRefreshingNetworksAndTokens: false
}
