// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// asset
import { USDCIconUrl } from './asset-icons'

// types
import { BraveWallet, WalletAccountType, WalletState } from '../../constants/types'
import { AllNetworksOptionDefault } from '../../options/network-filter-options'
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import { AllAccountsOption } from '../../options/account-filter-options'
import { mockMoonCatNFT } from './mock-asset-options'

// mocks
import { mockedErc20ApprovalTransaction, mockTransactionInfo } from './mock-transaction-info'
import { LAMPORTS_PER_SOL } from '../../common/constants/solana'

const mockAccount: WalletAccountType = {
  accountType: 'Primary',
  address: '0x15B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef1',
  coin: 60,
  deviceId: '',
  id: '0x15B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef1',
  name: 'Account 1',
  tokenBalanceRegistry: {
    '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346',
    '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69': '450346'
  },
  nativeBalanceRegistry: {
    [BraveWallet.MAINNET_CHAIN_ID]: '496917339073158043',
    [BraveWallet.GOERLI_CHAIN_ID]: '496917339073158043'
  },
  keyringId: 'default'
}

const mockAccount2: WalletAccountType = {
  accountType: 'Primary',
  address: '0x25B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2',
  coin: 60,
  deviceId: '',
  id: '0x25B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2',
  name: 'Account 2',
  tokenBalanceRegistry: {
    '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346',
    '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69': '450346'
  },
  nativeBalanceRegistry: {
    [BraveWallet.MAINNET_CHAIN_ID]: '496917339073158043',
    [BraveWallet.GOERLI_CHAIN_ID]: '496917339073158043'
  },
  keyringId: 'default'
}

const mockAccount3: WalletAccountType = {
  accountType: 'Primary',
  address: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef3',
  coin: 60,
  deviceId: '',
  id: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef3',
  name: 'Account 3',
  tokenBalanceRegistry: {
    '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346',
    '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69': '450346'
  },
  nativeBalanceRegistry: {
    [BraveWallet.MAINNET_CHAIN_ID]: '496917339073158043',
    [BraveWallet.GOERLI_CHAIN_ID]: '496917339073158043'
  },
  keyringId: 'default'
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
    origin: {
      scheme: 'https',
      host: 'app.uniswap.org',
      port: 443,
      nonceIfOpaque: undefined
    },
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
  fullTokenList: [
    {
      coingeckoId: 'usd-coin',
      contractAddress: '0x07865c6e87b9f70255377e024ace6630c1eaa37f',
      decimals: 6,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      logo: 'chrome://erc-token-images/usdc.png',
      name: 'USD Coin',
      symbol: 'USDC',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID
    },
    {
      coingeckoId: 'dai',
      contractAddress: '0xad6d458402f60fd3bd25163575031acdce07538d',
      decimals: 18,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      logo: 'chrome://erc-token-images/dai.png',
      name: 'DAI Stablecoin',
      symbol: 'DAI',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID
    }
  ],
  gasEstimates: undefined,
  hasIncorrectPassword: false,
  hasInitialized: true,
  isFetchingPortfolioPriceHistory: false,
  isFilecoinEnabled: false,
  isMetaMaskInstalled: false,
  isSolanaEnabled: false,
  solFeeEstimates: {
    fee: 0.000005 * LAMPORTS_PER_SOL as unknown as bigint
  },
  isWalletBackedUp: true,
  isWalletCreated: false,
  isWalletLocked: false,
  knownTransactions: [],
  pendingTransactions: [],
  portfolioPriceHistory: [],
  selectedAccount: mockAccount,
  selectedPendingTransaction: mockedErc20ApprovalTransaction,
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  transactions: {
    [mockAccount.address]: [mockTransactionInfo]
  },
  transactionSpotPrices: [
    {
      assetTimeframeChange: '',
      fromAsset: 'eth',
      price: '2581.2',
      toAsset: 'usd',
      contractAddress: '',
      chainId: '0x1'
    },
    {
      assetTimeframeChange: '',
      fromAsset: 'eth',
      price: '0',
      toAsset: 'usd',
      contractAddress: '',
      chainId: '0x1'
    },
    {
      assetTimeframeChange: '-0.18757681821254726',
      fromAsset: 'usdc',
      price: '0.999414',
      toAsset: 'usd',
      contractAddress: '0xusdc',
      chainId: '0x1'
    }
  ],
  userVisibleTokensInfo: [
    {
      coingeckoId: '',
      contractAddress: '',
      decimals: 18,
      isErc20: false,
      isErc721: false,
      isErc1155: false,
      isNft: false,
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
  transactionProviderErrorRegistry: {},
  selectedNetworkFilter: AllNetworksOptionDefault,
  selectedAssetFilter: HighToLowAssetsFilterOption.id,
  selectedAccountFilter: AllAccountsOption.id,
  defaultAccounts: [
    {
      address: mockAccount.address,
      coin: mockAccount.coin,
      name: mockAccount.name,
      isImported: false,
      hardware: undefined,
      keyringId: 'default'
    }
  ],
  onRampCurrencies: mockCurrencies,
  selectedCurrency: mockCurrency,
  passwordAttempts: 0,
  isLoadingCoinMarketData: false,
  coinMarketData: mockCoinMarketData,
  assetAutoDiscoveryCompleted: false,
  isNftPinningFeatureEnabled: false,
  isPanelV2FeatureEnabled: false
}
