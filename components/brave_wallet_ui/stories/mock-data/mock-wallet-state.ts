// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { LAMPORTS_PER_SOL } from '@solana/web3.js'

// types
import { BraveWallet, WalletAccountType, WalletState } from '../../constants/types'
import { AllNetworksOption } from '../../options/network-filter-options'
import { AllAssetsFilterOption } from '../../options/asset-filter-options'
import { mockMoonCatNFT } from './mock-asset-options'

// mocks
import { mockNetwork } from '../../common/constants/mocks'

const mockAccount: WalletAccountType = {
  accountType: 'Primary',
  address: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2',
  coin: 60,
  deviceId: '',
  id: '1',
  name: 'Account 1',
  tokenBalanceRegistry: {
    '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346',
    '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69': '450346'
  },
  nativeBalanceRegistry: {
    [BraveWallet.MAINNET_CHAIN_ID]: '496917339073158043',
    [BraveWallet.ROPSTEN_CHAIN_ID]: '496917339073158043'
  },
  keyringId: undefined
}

const mockNetworkList = [
  {
    'chainId': '0x1',
    'chainName': 'Ethereum Mainnet',
    'blockExplorerUrls': [
      'https://etherscan.io'
    ],
    'iconUrls': [],
    'rpcUrls': [
      'https://mainnet-infura.brave.com/f7106c838853428280fa0c585acc9485'
    ],
    'symbol': 'ETH',
    'symbolName': 'Ethereum',
    'decimals': 18,
    'coin': 60,
    'data': {
      'ethData': {
        'isEip1559': true
      }
    }
  },
  {
    'chainId': '0x4',
    'chainName': 'Rinkeby Test Network',
    'blockExplorerUrls': [
      'https://rinkeby.etherscan.io'
    ],
    'iconUrls': [],
    'rpcUrls': [
      'https://rinkeby-infura.brave.com/f7106c838853428280fa0c585acc9485'
    ],
    'symbol': 'ETH',
    'symbolName': 'Ethereum',
    'decimals': 18,
    'coin': 60,
    'data': {
      'ethData': {
        'isEip1559': true
      }
    }
  },
  {
    'chainId': '0x3',
    'chainName': 'Ropsten Test Network',
    'blockExplorerUrls': [
      'https://ropsten.etherscan.io'
    ],
    'iconUrls': [],
    'rpcUrls': [
      'https://ropsten-infura.brave.com/f7106c838853428280fa0c585acc9485'
    ],
    'symbol': 'ETH',
    'symbolName': 'Ethereum',
    'decimals': 18,
    'coin': 60,
    'data': {
      'ethData': {
        'isEip1559': true
      }
    }
  },
  {
    'chainId': '0x5',
    'chainName': 'Goerli Test Network',
    'blockExplorerUrls': [
      'https://goerli.etherscan.io'
    ],
    'iconUrls': [],
    'rpcUrls': [
      'https://goerli-infura.brave.com/f7106c838853428280fa0c585acc9485'
    ],
    'symbol': 'ETH',
    'symbolName': 'Ethereum',
    'decimals': 18,
    'coin': 60,
    'data': {
      'ethData': {
        'isEip1559': true
      }
    }
  },
  {
    'chainId': '0x2a',
    'chainName': 'Kovan Test Network',
    'blockExplorerUrls': [
      'https://kovan.etherscan.io'
    ],
    'iconUrls': [],
    'rpcUrls': [
      'https://kovan-infura.brave.com/f7106c838853428280fa0c585acc9485'
    ],
    'symbol': 'ETH',
    'symbolName': 'Ethereum',
    'decimals': 18,
    'coin': 60,
    'data': {
      'ethData': {
        'isEip1559': true
      }
    }
  },
  {
    'chainId': '0x539',
    'chainName': 'Localhost',
    'blockExplorerUrls': [
      'http://localhost:7545/'
    ],
    'iconUrls': [],
    'rpcUrls': [
      'http://localhost:7545/'
    ],
    'symbol': 'ETH',
    'symbolName': 'Ethereum',
    'decimals': 18,
    'coin': 60,
    'data': {
      'ethData': {
        'isEip1559': false
      }
    }
  }
]

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

export const mockWalletState: WalletState = {
  accounts: [
    mockAccount
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
      logo: 'chrome://erc-token-images/dai.png',
      name: 'DAI Stablecoin',
      symbol: 'DAI',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID
    },
    mockMoonCatNFT,
    { ...mockMoonCatNFT, tokenId: '0x52a5' },
    { ...mockMoonCatNFT, tokenId: '0x62a5' }
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
  networkList: mockNetworkList,
  pendingTransactions: [],
  portfolioPriceHistory: [],
  selectedAccount: mockAccount,
  selectedNetwork: mockNetworkList[0],
  selectedPendingTransaction: undefined,
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  transactions: {},
  transactionSpotPrices: [
    {
      assetTimeframeChange: '',
      fromAsset: 'eth',
      price: '2581.2',
      toAsset: 'usd'
    },
    {
      assetTimeframeChange: '',
      fromAsset: 'eth',
      price: '0',
      toAsset: 'usd'
    },
    {
      assetTimeframeChange: '-0.18757681821254726',
      fromAsset: 'usdc',
      price: '0.999414',
      toAsset: 'usd'
    }
  ],
  userVisibleTokensInfo: [
    {
      coingeckoId: '',
      contractAddress: '',
      decimals: 18,
      isErc20: false,
      isErc721: false,
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
      logo: 'chrome://erc-token-images/usdc.png',
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
      logo: 'chrome://erc-token-images/',
      name: 'Ethereum',
      symbol: 'ETH',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.ROPSTEN_CHAIN_ID
    },
    {
      coingeckoId: 'usd-coin',
      contractAddress: '0x07865c6E87B9F70255377e024ace6630C1Eaa37F',
      decimals: 6,
      isErc20: true,
      isErc721: false,
      logo: 'chrome://erc-token-images/usdc.png',
      name: 'USD Coin',
      symbol: 'USDC',
      tokenId: '',
      visible: true,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.ROPSTEN_CHAIN_ID
    },
    mockMoonCatNFT,
    { ...mockMoonCatNFT, tokenId: '0x52a5' },
    { ...mockMoonCatNFT, tokenId: '0x62a5' },
    { ...mockMoonCatNFT, tokenId: '0x72a5' },
    { ...mockMoonCatNFT, tokenId: '0x82a5' }
  ],
  transactionProviderErrorRegistry: {},
  defaultNetworks: [mockNetwork],
  isTestNetworksEnabled: true,
  selectedCoin: BraveWallet.CoinType.ETH,
  selectedNetworkFilter: AllNetworksOption,
  selectedAssetFilter: AllAssetsFilterOption,
  defaultAccounts: [
    {
      address: mockAccount.address,
      coin: mockAccount.coin,
      name: mockAccount.name,
      isImported: false,
      hardware: undefined,
      keyringId: undefined
    }
  ],
  onRampCurrencies: mockCurrencies,
  selectedCurrency: mockCurrency,
  passwordAttempts: 0
}
