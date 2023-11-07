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
import { LAMPORTS_PER_SOL } from '../../common/constants/solana'
import { mockMoonCatNFT, mockErc20TokensList } from './mock-asset-options'


export const mockWalletState: WalletState = {
  activeOrigin: {
    originSpec: 'https://app.uniswap.org',
    eTldPlusOne: 'uniswap.org'
  },
  addUserAssetError: false,
  connectedAccounts: [],
  defaultCurrencies: {
    crypto: 'ETH',
    fiat: 'USD'
  },
  defaultEthereumWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
  defaultSolanaWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
  fullTokenList: mockErc20TokensList,
  gasEstimates: undefined,
  hasIncorrectPassword: false,
  hasInitialized: true,
  isFilecoinEnabled: false,
  isMetaMaskInstalled: false,
  isSolanaEnabled: false,
  isBitcoinEnabled: false,
  isZCashEnabled: false,
  isAnkrBalancesFeatureEnabled: false,
  solFeeEstimates: {
    fee: (0.000005 * LAMPORTS_PER_SOL) as unknown as bigint
  },
  isWalletBackedUp: true,
  isWalletCreated: false,
  isWalletLocked: false,
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
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
  isPanelV2FeatureEnabled: false,
  hidePortfolioBalances: false,
  hidePortfolioGraph: false,
  removedFungibleTokenIds: [],
  removedNonFungibleTokenIds: [],
  removedNonFungibleTokens: [],
  deletedNonFungibleTokenIds: [],
  deletedNonFungibleTokens: [],
  hidePortfolioNFTsTab: false,
  filteredOutPortfolioNetworkKeys: [],
  filteredOutPortfolioAccountAddresses: [],
  hidePortfolioSmallBalances: false,
  selectedGroupAssetsByItem: AccountsGroupByOption.id,
  showNetworkLogoOnNfts: false,
  isRefreshingNetworksAndTokens: false,
  importAccountError: false
}
