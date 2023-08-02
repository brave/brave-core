// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// asset
import { USDCIconUrl } from './asset-icons'

// types
import {
  BraveWallet,
  CoinType,
  WalletState
} from '../../constants/types'
import { AllNetworksOptionDefault } from '../../options/network-filter-options'
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import { AllAccountsOptionUniqueKey } from '../../options/account-filter-options'
import { AccountsGroupByOption } from '../../options/group-assets-by-options'

// mocks
import { LAMPORTS_PER_SOL } from '../../common/constants/solana'
import { mockMoonCatNFT, mockErc20TokensList } from './mock-asset-options'

const mockAccount: BraveWallet.AccountInfo = {
  address: '0x15B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef1',
  accountId: {
    coin: 60,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0x15B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef1',
    bitcoinAccountIndex: 0,
    uniqueKey: '0x15B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef1'
  },
  name: 'Account 1',
  hardware: undefined
}

const mockAccount2: BraveWallet.AccountInfo = {
  address: '0x25B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2',
  accountId: {
    coin: 60,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0x25B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2',
    bitcoinAccountIndex: 0,
    uniqueKey: '0x25B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2'
  },
  name: 'Account 2',
  hardware: undefined
}

const mockAccount3: BraveWallet.AccountInfo = {
  address: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef3',
  accountId: {
    coin: 60,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef3',
    bitcoinAccountIndex: 0,
    uniqueKey: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef3'
  },
  name: 'Account 3',
  hardware: undefined
}

const mockCurrency: BraveWallet.OnRampCurrency = {
  currencyCode: 'USD',
  currencyName: 'United States Dollar',
  providers: []
}

const mockCurrencies: BraveWallet.OnRampCurrency[] = [
  mockCurrency,
  {
    currencyCode: 'EUR',
    currencyName: 'Euro',
    providers: []
  },
  {
    currencyCode: 'GBP',
    currencyName: 'British Pound Sterling',
    providers: []
  }
]

const mockCoinMarketData: BraveWallet.CoinMarket[] = [
  {
    id: 'bitcoin',
    symbol: 'btc',
    name: 'Bitcoin',
    image: 'https://assets.cgproxy.brave.com/coins/images/1/large/bitcoin.png?1547033579',
    marketCap: 768537918492,
    marketCapRank: 1,
    currentPrice: 40408,
    totalVolume: 29207260045,
    priceChange24h: 1878.92,
    priceChangePercentage24h: 4.87667
  },
  {
    id: 'ethereum',
    symbol: 'eth',
    name: 'Ethereum',
    image: 'https://assets.cgproxy.brave.com/coins/images/279/large/ethereum.png?1595348880',
    currentPrice: 3000.46,
    marketCap: 361354711703,
    marketCapRank: 2,
    totalVolume: 19247516322,
    priceChange24h: 170.9,
    priceChangePercentage24h: 6.03982
  },
  {
    id: 'tether',
    symbol: 'usdt',
    name: 'Tether',
    image: 'https://assets.cgproxy.brave.com/coins/images/325/large/Tether-logo.png?1598003707',
    currentPrice: 0.999981,
    marketCap: 83153567337,
    marketCapRank: 3,
    totalVolume: 60387266495,
    priceChange24h: -0.00077149716,
    priceChangePercentage24h: -0.07709
  },
  {
    id: 'binancecoin',
    symbol: 'bnb',
    name: 'BNB',
    image: 'https://assets.cgproxy.brave.com/coins/images/825/large/bnb-icon2_2x.png?1644979850',
    currentPrice: 400.1,
    marketCap: 67256828191,
    marketCapRank: 4,
    totalVolume: 1627664095,
    priceChange24h: 12.97,
    priceChangePercentage24h: 3.34962
  }
]

export const mockWalletState: WalletState = {
  accounts: [
    mockAccount,
    mockAccount2,
    mockAccount3
  ],
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
  favoriteApps: [],
  fullTokenList: mockErc20TokensList,
  gasEstimates: undefined,
  hasIncorrectPassword: false,
  hasInitialized: true,
  isFilecoinEnabled: false,
  isMetaMaskInstalled: false,
  isSolanaEnabled: false,
  isBitcoinEnabled: false,
  solFeeEstimates: {
    fee: 0.000005 * LAMPORTS_PER_SOL as unknown as bigint
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
      coin: CoinType.ETH,
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
      coin: CoinType.ETH,
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
      coin: CoinType.ETH,
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
      coin: CoinType.ETH,
      chainId: BraveWallet.GOERLI_CHAIN_ID
    },
    mockMoonCatNFT
  ],
  selectedNetworkFilter: AllNetworksOptionDefault,
  selectedAssetFilter: HighToLowAssetsFilterOption.id,
  selectedAccountFilter: AllAccountsOptionUniqueKey,
  onRampCurrencies: mockCurrencies,
  selectedCurrency: mockCurrency,
  passwordAttempts: 0,
  isLoadingCoinMarketData: false,
  coinMarketData: mockCoinMarketData,
  assetAutoDiscoveryCompleted: false,
  isNftPinningFeatureEnabled: false,
  isPanelV2FeatureEnabled: false,
  hidePortfolioBalances: false,
  hidePortfolioGraph: false,
  removedFungibleTokenIds: [],
  removedNonFungibleTokenIds: [],
  removedNonFungibleTokens: [],
  hidePortfolioNFTsTab: false,
  filteredOutPortfolioNetworkKeys: [],
  filteredOutPortfolioAccountAddresses: [],
  hidePortfolioSmallBalances: false,
  selectedGroupAssetsByItem: AccountsGroupByOption.id,
  showNetworkLogoOnNfts: false,
  isRefreshingNetworksAndTokens: false
}
