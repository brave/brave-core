import { mockNetwork } from '../../common/constants/mocks'
import { BraveWallet, WalletAccountType, WalletState } from '../../constants/types'
import { AllNetworksOption } from '../../options/network-filter-options'

const mockAccount: WalletAccountType = {
  accountType: 'Primary',
  address: '0x35B83cC0e0fA0bFd21181fd2e07Ad900EA8D6ef2',
  coin: 60,
  deviceId: '',
  id: '1',
  name: 'Account 1',
  tokenBalanceRegistry: {
    '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346'
  },
  nativeBalanceRegistry: {
    [BraveWallet.MAINNET_CHAIN_ID]: '496917339073158043',
    [BraveWallet.ROPSTEN_CHAIN_ID]: '496917339073158043'
  }
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

export const mockWalletState: WalletState = {
  accounts: [
    mockAccount
  ],
  activeOrigin: {
    origin: 'https://app.uniswap.org',
    eTldPlusOne: 'uniswap.org'
  },
  addUserAssetError: false,
  connectedAccounts: [],
  defaultCurrencies: {
    crypto: 'ETH',
    fiat: 'USD'
  },
  defaultWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
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
    }
  ],
  gasEstimates: undefined,
  hasIncorrectPassword: false,
  hasInitialized: true,
  isFetchingPortfolioPriceHistory: false,
  isFilecoinEnabled: false,
  isMetaMaskInstalled: false,
  isSolanaEnabled: false,
  isWalletBackedUp: true,
  isWalletCreated: false,
  isWalletLocked: false,
  knownTransactions: [],
  networkList: mockNetworkList,
  pendingTransactions: [],
  portfolioPriceHistory: [],
  selectedAccount: mockAccount,
  selectedNetwork: mockNetworkList[2],
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
    }
  ],
  transactionProviderErrorRegistry: {},
  defaultNetworks: [mockNetwork],
  isTestNetworksEnabled: true,
  selectedCoin: BraveWallet.CoinType.ETH,
  selectedNetworkFilter: AllNetworksOption
}
