// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */
import { assert } from 'chrome://resources/js/assert.js'

// redux
import { createStore, combineReducers } from 'redux'
import { createWalletReducer } from '../../slices/wallet.slice'

// types
import { BraveWallet, CommonNftMetadata } from '../../../constants/types'
import { WalletActions } from '../../actions'
import type WalletApiProxy from '../../wallet_api_proxy'

// utils
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { deserializeTransaction } from '../../../utils/model-serialization-utils'
import { getAssetIdKey } from '../../../utils/asset-utils'
import Amount from '../../../utils/amount'

// mocks
import { mockWalletState } from '../../../stories/mock-data/mock-wallet-state'
import { mockedMnemonic } from '../../../stories/mock-data/user-accounts'
import {
  NativeAssetBalanceRegistry,
  TokenBalanceRegistry,
  mockAccount,
  mockEthAccountInfo,
  mockFilecoinAccountInfo,
  mockFilecoinMainnetNetwork,
  mockOnRampCurrencies,
  mockSolanaAccountInfo,
  mockSolanaMainnetNetwork
} from '../../constants/mocks'
import {
  mockEthMainnet,
  mockNetworks
} from '../../../stories/mock-data/mock-networks'
import {
  mockAccountAssetOptions,
  mockBasicAttentionToken,
  mockErc20TokensList,
  mockErc721Token,
  mockMoonCatNFT,
  mockSplBat,
  mockSplNft,
  mockSplUSDC,
  mockTokensList
} from '../../../stories/mock-data/mock-asset-options'
import {
  mockFilSendTransaction,
  mockTransactionInfo,
  mockedErc20ApprovalTransaction
} from '../../../stories/mock-data/mock-transaction-info'
import { blockchainTokenEntityAdaptor } from '../../slices/entities/blockchain-token.entity'
import { findAccountByUniqueKey } from '../../../utils/account-utils'
import { mockNFTMetadata } from '../../../stories/mock-data/mock-nft-metadata'
import {
  coinMarketMockData //
} from '../../../stories/mock-data/mock-coin-market-data'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { WalletApiDataOverrides } from '../../../constants/testing_types'
import {
  mockAddChainRequest,
  mockDecryptRequest,
  mockGetEncryptionPublicKeyRequest,
  mockSignMessageError,
  mockSignMessageRequest,
  mockSwitchChainRequest
} from '../../../stories/mock-data/mock-eth-requests'
import { mockDappsListMap } from '../../../mocks/mock-dapps-list'

export const makeMockedStoreWithSpy = () => {
  const store = createStore(
    combineReducers({
      wallet: createWalletReducer(mockWalletState)
    })
  )

  const areWeTestingWithJest = process.env.JEST_WORKER_ID !== undefined

  if (areWeTestingWithJest) {
    const dispatchSpy = jest.fn(store.dispatch)
    const ogDispatch = store.dispatch
    store.dispatch = ((args: any) => {
      ogDispatch(args)
      dispatchSpy?.(args)
    }) as any
    return { store, dispatchSpy }
  }

  return { store }
}

export class MockedWalletApiProxy {
  store = makeMockedStoreWithSpy().store

  defaultBaseCurrency: string = 'usd'
  selectedAccountId: BraveWallet.AccountId = mockAccount.accountId
  accountInfos: BraveWallet.AccountInfo[] = [
    mockAccount,
    mockEthAccountInfo,
    mockSolanaAccountInfo,
    mockFilecoinAccountInfo
  ]

  selectedNetwork: BraveWallet.NetworkInfo = mockEthMainnet

  chainIdsForCoins: Record<BraveWallet.CoinType, string> = {
    [BraveWallet.CoinType.ETH]: BraveWallet.MAINNET_CHAIN_ID,
    [BraveWallet.CoinType.SOL]: BraveWallet.SOLANA_MAINNET,
    [BraveWallet.CoinType.FIL]: BraveWallet.FILECOIN_MAINNET
  }

  chainsForCoins: Record<BraveWallet.CoinType, BraveWallet.NetworkInfo> = {
    [BraveWallet.CoinType.ETH]: mockEthMainnet,
    [BraveWallet.CoinType.SOL]: mockSolanaMainnetNetwork,
    [BraveWallet.CoinType.FIL]: mockFilecoinMainnetNetwork
  }

  networks: BraveWallet.NetworkInfo[] = mockNetworks

  blockchainTokens: BraveWallet.BlockchainToken[] = [
    ...mockErc20TokensList,
    mockErc721Token,
    mockSplNft,
    mockSplBat,
    mockSplUSDC
  ]

  userAssets: BraveWallet.BlockchainToken[] = mockAccountAssetOptions

  evmSimulationResponse: BraveWallet.EVMSimulationResponse | null = null

  svmSimulationResponse: BraveWallet.SolanaSimulationResponse | null = null

  txSimulationOptInStatus: BraveWallet.BlowfishOptInStatus =
    BraveWallet.BlowfishOptInStatus.kAllowed

  /**
   * balance = [accountAddress][chainId]
   */
  nativeBalanceRegistry: NativeAssetBalanceRegistry = {
    [mockAccount.address]: {
      [BraveWallet.MAINNET_CHAIN_ID]: '0' // 0 ETH
    }
  }

  /**
   * balance = [accountAddress][assetEntityId]
   */
  tokenBalanceRegistry: TokenBalanceRegistry = {
    [mockAccount.address]: {
      // 0 BAT
      [blockchainTokenEntityAdaptor.selectId(mockBasicAttentionToken)]: '0'
    }
  }

  mockZeroExQuote = {
    price: '1705.399509',
    guaranteedPrice: '',
    to: '',
    data: '',
    value: '124067000000000000',
    gas: '280000',
    estimatedGas: '280000',
    gasPrice: '2000000000',
    protocolFee: '0',
    minimumProtocolFee: '0',
    sellTokenAddress: '0x07865c6e87b9f70255377e024ace6630c1eaa37f',
    buyTokenAddress: '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
    buyAmount: '211599920',
    sellAmount: '124067000000000000',
    allowanceTarget: '0x0000000000000000000000000000000000000000',
    sellTokenToEthRate: '1',
    buyTokenToEthRate: '1720.180416',
    estimatedPriceImpact: '0.0782',
    sources: [],
    fees: {
      zeroExFee: undefined
    }
  }

  mockZeroExTransaction = {
    allowanceTarget: '',
    price: '',
    guaranteedPrice: '',
    to: '',
    data: '',
    value: '',
    gas: '0',
    estimatedGas: '0',
    gasPrice: '0',
    protocolFee: '0',
    minimumProtocolFee: '0',
    buyTokenAddress: '',
    sellTokenAddress: '',
    buyAmount: '0',
    sellAmount: '0',
    sellTokenToEthRate: '1',
    buyTokenToEthRate: '1',
    estimatedPriceImpact: '0.0782',
    sources: [],
    fees: {
      zeroExFee: undefined
    }
  }

  transactionInfos: BraveWallet.TransactionInfo[] = [
    deserializeTransaction(mockTransactionInfo),
    mockFilSendTransaction as BraveWallet.TransactionInfo,
    deserializeTransaction(mockedErc20ApprovalTransaction)
  ]

  // name service lookups
  requireOffchainConsent: number = BraveWallet.ResolveMethod.kAsk

  private pendingAddChainRequests = [mockAddChainRequest]
  private pendingSwitchChainRequests: BraveWallet.SwitchChainRequest[] = [
    mockSwitchChainRequest
  ]

  private pendingDecryptRequests: BraveWallet.DecryptRequest[] = [
    mockDecryptRequest
  ]

  private pendingEncryptionPublicKeyRequests = [
    mockGetEncryptionPublicKeyRequest
  ]

  private signTransactionRequests: BraveWallet.SignTransactionRequest[] = []

  private signAllTransactionsRequests =
    [] as BraveWallet.SignAllTransactionsRequest[]

  constructor(overrides?: WalletApiDataOverrides | undefined) {
    this.applyOverrides(overrides)
  }

  applyOverrides(overrides?: WalletApiDataOverrides | undefined) {
    if (!overrides) {
      return
    }

    this.selectedAccountId =
      overrides.selectedAccountId ?? this.selectedAccountId
    this.chainIdsForCoins = overrides.chainIdsForCoins ?? this.chainIdsForCoins
    this.networks = overrides.networks ?? this.networks
    this.defaultBaseCurrency =
      overrides.defaultBaseCurrency ?? this.defaultBaseCurrency
    this.transactionInfos = overrides.transactionInfos ?? this.transactionInfos
    this.blockchainTokens = overrides.blockchainTokens ?? this.blockchainTokens
    this.userAssets = overrides.userAssets ?? this.userAssets
    this.accountInfos = overrides.accountInfos ?? this.accountInfos
    this.nativeBalanceRegistry =
      overrides.nativeBalanceRegistry ?? this.nativeBalanceRegistry
    this.tokenBalanceRegistry =
      overrides.tokenBalanceRegistry ?? this.tokenBalanceRegistry
    this.evmSimulationResponse =
      overrides.evmSimulationResponse ?? this.evmSimulationResponse
    this.svmSimulationResponse =
      overrides.svmSimulationResponse ?? this.svmSimulationResponse
    this.txSimulationOptInStatus =
      overrides.simulationOptInStatus ?? this.txSimulationOptInStatus
    this.signTransactionRequests =
      overrides.signTransactionRequests ?? this.signTransactionRequests
    this.signAllTransactionsRequests =
      overrides.signAllTransactionsRequests ?? this.signAllTransactionsRequests
  }

  assetsRatioService: Partial<
    InstanceType<typeof BraveWallet.AssetRatioServiceInterface>
  > = {
    getBuyUrlV1: async (
      provider,
      chainId,
      address,
      symbol,
      amount,
      currencyCode
    ) => {
      if (
        !provider ||
        !chainId ||
        !address ||
        !symbol ||
        !amount ||
        !currencyCode
      ) {
        return {
          url: '',
          error: 'missing param(s)'
        }
      }
      return {
        url: `provider=${
          provider //
        }&chainId=${
          chainId //
        }&address=${
          address //
        }&symbol=${
          symbol //
        }&amount=${
          amount //
        }&currencyCode=${
          currencyCode //
        }`,
        error: null
      }
    }
  }

  blockchainRegistry: Partial<
    InstanceType<typeof BraveWallet.BlockchainRegistryInterface>
  > = {
    getAllTokens: async (chainId: string, coin: number) => {
      return {
        tokens: this.blockchainTokens.filter(
          (t) => t.chainId === chainId && t.coin === coin
        )
      }
    },

    getBuyTokens: async (provider, chainId) => {
      return {
        tokens: this.blockchainTokens.filter((t) => t.chainId === chainId)
      }
    },

    getOnRampCurrencies: async () => {
      return {
        currencies: mockOnRampCurrencies
      }
    },

    getTopDapps: async (chainId, coin) => {
      switch (chainId) {
        // supporting only ethereum mainnet and solana in this mock
        case BraveWallet.MAINNET_CHAIN_ID: {
          const parser = createDappParserForRange(
            mockDappsListMap.ethereum.range
          )
          return {
            dapps: mockDappsListMap.ethereum.results.map(parser)
          }
        }
        case BraveWallet.SOLANA_MAINNET: {
          const parser = createDappParserForRange(mockDappsListMap.solana.range)
          return {
            dapps: mockDappsListMap.solana.results.map(parser)
          }
        }
        default:
          return { dapps: [] }
      }
    }
  }

  braveWalletService: Partial<
    InstanceType<typeof BraveWallet.BraveWalletServiceInterface>
  > = {
    getUserAssets: async (chainId: string, coin: BraveWallet.CoinType) => {
      return {
        tokens: this.userAssets.filter(
          (t) => t.chainId === chainId && t.coin === coin
        )
      }
    },
    getDefaultBaseCurrency: async () => ({
      currency: this.defaultBaseCurrency
    }),
    setDefaultBaseCurrency: async (currency: string) => {
      this.defaultBaseCurrency = currency
    },
    getActiveOrigin: async () => {
      return {
        originInfo: {
          originSpec: 'https://brave.com',
          eTldPlusOne: 'brave.com'
        }
      }
    },
    getNetworkForSelectedAccountOnActiveOrigin: async () => {
      return { network: this.selectedNetwork }
    },
    isBase58EncodedSolanaPubkey: async (key) => {
      return {
        result: true
      }
    },
    getBalanceScannerSupportedChains: async () => {
      return {
        chainIds: this.networks.map((n) => n.chainId)
      }
    },
    ensureSelectedAccountForChain: async (coin, chainId) => {
      const foundAccount = findAccountByUniqueKey(
        this.accountInfos,
        this.selectedAccountId.uniqueKey
      )

      return {
        accountId:
          foundAccount?.accountId.coin === coin
            ? foundAccount.accountId
            : this.accountInfos.find((a) => a.accountId.coin === coin)
                ?.accountId ?? null
      }
    },
    setNetworkForSelectedAccountOnActiveOrigin: async (chainId) => {
      if (this.selectedNetwork.chainId === chainId) {
        return {
          success: true
        }
      }

      const net = this.networks.find((n) => n.chainId === chainId)

      if (net) {
        this.selectedNetwork = net
      }

      return {
        success: !!net
      }
    },
    getPendingAddSuggestTokenRequests: async () => {
      return {
        requests: [{ origin: mockOriginInfo, token: mockBasicAttentionToken }]
      }
    },
    removeUserAsset: async (token) => {
      const tokenId = getAssetIdKey(token)
      this.userAssets = this.userAssets.filter(
        (t) => getAssetIdKey(t) !== tokenId
      )
      return {
        success: true
      }
    },
    addUserAsset: async (token) => {
      this.userAssets = this.userAssets.concat(token)
      return {
        success: true
      }
    },
    setUserAssetVisible: async (token, visible) => {
      const tokenId = getAssetIdKey(token)
      this.userAssets = this.userAssets.map((t) =>
        getAssetIdKey(t) === tokenId ? { ...t, visible } : t
      )
      return { success: true }
    },
    getPendingDecryptRequests: async () => {
      return {
        requests: this.pendingDecryptRequests
      }
    },
    notifyDecryptRequestProcessed: (requestId, approved) => {
      this.pendingDecryptRequests = this.pendingDecryptRequests.filter(
        (req) => req.requestId !== requestId
      )
    },
    getPendingGetEncryptionPublicKeyRequests: async () => {
      return {
        requests: this.pendingEncryptionPublicKeyRequests
      }
    },
    notifyGetPublicKeyRequestProcessed: (requestId, approved) => {
      this.pendingEncryptionPublicKeyRequests =
        this.pendingEncryptionPublicKeyRequests.filter(
          (req) => req.requestId !== requestId
        )
    },
    getPendingSignTransactionRequests: async () => {
      return {
        requests: this.signTransactionRequests
      }
    },
    getPendingSignAllTransactionsRequests: async () => {
      return {
        requests: this.signAllTransactionsRequests
      }
    },
    notifySignTransactionRequestProcessed: (approved, id, signature, error) => {
      this.signTransactionRequests = this.signTransactionRequests.filter(
        (req) => req.id !== id
      )
    },
    notifySignAllTransactionsRequestProcessed: (
      approved,
      id,
      signatures,
      error
    ) => {
      this.signAllTransactionsRequests =
        this.signAllTransactionsRequests.filter((req) => req.id !== id)
    },
    getPendingSignMessageRequests: async () => {
      return {
        requests: [mockSignMessageRequest]
      }
    },
    getPendingSignMessageErrors: async () => {
      return {
        errors: [mockSignMessageError]
      }
    },
    getTransactionSimulationOptInStatus: async () => {
      return {
        status: this.txSimulationOptInStatus
      }
    }
  }

  swapService: Partial<InstanceType<typeof BraveWallet.SwapServiceInterface>> =
    {
      getTransaction: async (
        params: BraveWallet.SwapTransactionParamsUnion
      ): Promise<{
        response: BraveWallet.SwapTransactionUnion | null
        error: BraveWallet.SwapErrorUnion | null
        errorString: string
      }> => {
        const { zeroExTransactionParams } = params
        if (!zeroExTransactionParams) {
          return {
            response: null,
            error: null,
            errorString: 'missing params'
          }
        }

        const { fromToken, toToken, fromAmount, toAmount } =
          zeroExTransactionParams

        return {
          error: null,
          response: {
            zeroExTransaction: {
              ...this.mockZeroExQuote,
              buyTokenAddress: toToken,
              sellTokenAddress: fromToken,
              buyAmount: toAmount || '',
              sellAmount: fromAmount || '',
              price: '1'
            },
            jupiterTransaction: undefined,
            lifiTransaction: undefined
          },
          errorString: ''
        }
      },

      getQuote: async (
        params: BraveWallet.SwapQuoteParams
      ): Promise<{
        response: BraveWallet.SwapQuoteUnion | null
        fees: BraveWallet.SwapFees | null
        error: BraveWallet.SwapErrorUnion | null
        errorString: string
      }> => ({
        response: {
          zeroExQuote: this.mockZeroExQuote,
          jupiterQuote: undefined,
          lifiQuote: undefined
        },
        fees: {
          feeParam: '0.00875',
          feePct: '0.875',
          discountPct: '0',
          effectiveFeePct: '0.875',
          discountCode: BraveWallet.SwapDiscountCode.kNone
        },
        error: null,
        errorString: ''
      })
    }

  keyringService: Partial<
    InstanceType<typeof BraveWallet.KeyringServiceInterface>
  > = {
    getAllAccounts: async (): Promise<{
      allAccounts: BraveWallet.AllAccountsInfo
    }> => {
      const selectedAccount = findAccountByUniqueKey(
        this.accountInfos,
        this.selectedAccountId.uniqueKey
      )
      assert(selectedAccount)
      const allAccounts: BraveWallet.AllAccountsInfo = {
        accounts: this.accountInfos,
        selectedAccount: selectedAccount,
        ethDappSelectedAccount: selectedAccount,
        solDappSelectedAccount: mockSolanaAccountInfo
      }
      return { allAccounts }
    },
    validatePassword: async (password: string) => ({
      result: password === 'password'
    }),
    lock: () => {
      this.store.dispatch(WalletActions.locked())
      alert('wallet locked')
    },
    encodePrivateKeyForExport: async (
      accountId: BraveWallet.AccountId,
      password: string
    ) =>
      password === 'password'
        ? { privateKey: 'secret-private-key' }
        : { privateKey: '' },
    getWalletMnemonic: async (password) => {
      return password === 'password'
        ? { mnemonic: mockedMnemonic }
        : { mnemonic: '' }
    },
    getChecksumEthAddress: async (address) => {
      return {
        checksumAddress: address.toLocaleLowerCase()
      }
    },
    setSelectedAccount: async (accountId) => {
      const validId = !!this.accountInfos.find(
        (a) => a.accountId.uniqueKey === accountId.uniqueKey
      )

      if (validId) {
        this.selectedAccountId = accountId
      } else {
        console.log('invalid id: ' + accountId.uniqueKey)
      }

      return {
        success: validId
      }
    },
    unlock: async (password) => {
      return { success: password === 'password' }
    }
  }

  ethTxManagerProxy: Partial<
    InstanceType<typeof BraveWallet.EthTxManagerProxyInterface>
  > = {
    getGasEstimation1559: async () => {
      return {
        estimation: {
          slowMaxPriorityFeePerGas: '0',
          slowMaxFeePerGas: '0',
          avgMaxPriorityFeePerGas: '0',
          avgMaxFeePerGas: '0',
          fastMaxPriorityFeePerGas: '0',
          fastMaxFeePerGas: '0',
          baseFeePerGas: '0'
        } as BraveWallet.GasEstimation1559 | null
      }
    }
  }

  braveWalletP3A: InstanceType<typeof BraveWallet.BraveWalletP3AInterface> = {
    reportOnboardingAction: (action) => {
      console.log(`reporting onboarding action: ${action}`)
    },
    reportJSProvider: (providerType, coinType, allowProviderOverwrite) => {
      console.log(
        `reporting JS provider: ${JSON.stringify(
          { providerType, coinType, allowProviderOverwrite },
          undefined,
          2
        )}`
      )
    },
    recordActiveWalletCount(count, coinType) {
      console.log(`active wallet count: ${count} for ${coinType}`)
    },
    recordNFTGalleryView(nftCount) {
      console.log(`viewing nft gallery with ${nftCount} nfts`)
    }
  }

  assetRatioService: Partial<
    InstanceType<typeof BraveWallet.AssetRatioServiceInterface>
  > = {
    getPrice: async (fromAssets, toAssets, timeframe) => {
      return {
        success: true,
        values: [
          {
            assetTimeframeChange: '1',
            fromAsset: fromAssets[0],
            toAsset: toAssets[0],
            price: '1234.56'
          }
        ]
      }
    },
    getCoinMarkets: async (vsAsset: string, limit: number) => {
      return {
        success: true,
        values: coinMarketMockData
      }
    }
  }

  jsonRpcService: Partial<
    InstanceType<typeof BraveWallet.JsonRpcServiceInterface>
  > = {
    getAllNetworks: async () => {
      return { networks: this.networks }
    },
    getHiddenNetworks: async () => {
      return { chainIds: [] }
    },
    getDefaultChainId: async (coin) => {
      return { chainId: this.chainIdsForCoins[coin] }
    },
    getNetwork: async (coin) => {
      return { network: this.chainsForCoins[coin] }
    },
    setNetwork: async (chainId, coin, origin) => {
      this.chainIdsForCoins[coin] = chainId
      const foundNetwork = this.networks.find(
        (net) => net.chainId === chainId && net.coin === coin
      )

      if (!foundNetwork) {
        throw new Error(
          `Could net find a mocked network to use for chainId: ${
            chainId //
          } & coin: ${
            coin //
          }`
        )
      }

      this.chainsForCoins[coin] = foundNetwork
      return { success: true }
    },
    getPendingAddChainRequests: async () => {
      return {
        requests: this.pendingAddChainRequests
      }
    },
    addEthereumChainRequestCompleted: (chainId, approved) => {
      this.pendingAddChainRequests = this.pendingAddChainRequests.filter(
        (req) => req.networkInfo.chainId !== chainId
      )
    },
    getPendingSwitchChainRequests: async () => {
      return {
        requests: this.pendingSwitchChainRequests
      }
    },
    notifySwitchChainRequestProcessed: (requestId, approved) => {
      const request = this.pendingSwitchChainRequests.find(
        (req) => req.requestId === requestId
      )

      if (request) {
        this.pendingSwitchChainRequests =
          this.pendingSwitchChainRequests.filter(
            (req) => req.requestId !== requestId
          )
        this.braveWalletService.setNetworkForSelectedAccountOnActiveOrigin?.(
          request.chainId
        )
      }
    },
    // Native asset balances
    getBalance: async (address: string, coin: number, chainId: string) => {
      return {
        balance: this.nativeBalanceRegistry?.[address]?.[chainId] || '0',
        error: 0,
        errorMessage: ''
      }
    },
    getSolanaBalance: async (pubkey: string, chainId: string) => {
      const balance = BigInt(this.nativeBalanceRegistry[pubkey]?.[chainId] ?? 0)
      return {
        balance,
        error: 0,
        errorMessage: ''
      }
    },
    // Token balances
    getERC20TokenBalance: async (contract, address, chainId) => {
      return {
        balance:
          this.tokenBalanceRegistry[address]?.[
            blockchainTokenEntityAdaptor.selectId({
              coin: BraveWallet.CoinType.ETH,
              chainId,
              contractAddress: contract,
              isErc721: false,
              tokenId: '',
              isNft: false
            })
          ] || '0',
        error: 0,
        errorMessage: ''
      }
    },
    getERC721TokenBalance: async (
      contractAddress,
      tokenId,
      accountAddress,
      chainId
    ) => {
      return {
        balance:
          this.tokenBalanceRegistry[accountAddress]?.[
            blockchainTokenEntityAdaptor.selectId({
              coin: BraveWallet.CoinType.ETH,
              chainId,
              contractAddress: contractAddress,
              isErc721: true,
              tokenId,
              isNft: false
            })
          ] || '0',
        error: 0,
        errorMessage: ''
      }
    },
    getERC1155TokenBalance: async (
      contractAddress,
      tokenId,
      accountAddress,
      chainId
    ) => {
      return {
        balance:
          this.tokenBalanceRegistry[accountAddress]?.[
            blockchainTokenEntityAdaptor.selectId({
              coin: BraveWallet.CoinType.ETH,
              chainId,
              contractAddress: contractAddress,
              isErc721: true,
              tokenId,
              isNft: false
            })
          ] || '0',
        error: 0,
        errorMessage: ''
      }
    },
    getSPLTokenAccountBalance: async (
      walletAddress,
      tokenMintAddress,
      chainId
    ) => {
      return {
        amount:
          this.tokenBalanceRegistry[walletAddress]?.[
            blockchainTokenEntityAdaptor.selectId({
              coin: BraveWallet.CoinType.ETH,
              chainId,
              contractAddress: tokenMintAddress,
              isErc721: false,
              tokenId: '',
              isNft: true
            })
          ] || '0',
        decimals: 9,
        uiAmountString: '',
        error: 0,
        errorMessage: ''
      }
    },
    getSPLTokenBalances: async (pubkey, chainId) => {
      const balances = Object.keys(this.tokenBalanceRegistry?.[pubkey])
        .filter((tokenId) => tokenId.includes(chainId))
        .map((tokenIdentifier) => {
          const token = this.blockchainTokens.find(
            (t) => getAssetIdKey(t) === tokenIdentifier
          )

          const amount =
            this.tokenBalanceRegistry[pubkey][tokenIdentifier] || '0'

          return {
            amount: this.tokenBalanceRegistry[pubkey][tokenIdentifier] || '0',
            decimals: token?.decimals ?? 1,
            mint: token?.contractAddress ?? '',
            uiAmount: amount
          }
        })
      return {
        balances,
        error: 0,
        errorMessage: ''
      }
    },
    getERC20TokenBalances: async (contracts, address, chainId) => {
      const balances = Object.keys(this.tokenBalanceRegistry?.[address])
        .filter((tokenId) => tokenId.includes(chainId))
        .map((tokenIdentifier) => {
          const token = this.blockchainTokens.find(
            (t) => getAssetIdKey(t) === tokenIdentifier
          )

          const amount =
            this.tokenBalanceRegistry[address][tokenIdentifier] || '0'

          return {
            balance: amount,
            contractAddress: token?.contractAddress || ''
          }
        })
      return {
        balances,
        error: 0,
        errorMessage: ''
      }
    },
    // Allowances
    getERC20TokenAllowance: async (
      contract,
      ownerAddress,
      spenderAddress,
      chainId
    ) => {
      return {
        allowance: '1000000000000000000', // 1 unit
        error: BraveWallet.ProviderError.kSuccess,
        errorMessage: ''
      }
    },
    // NFT Metadata
    getERC721Metadata: async (contract, tokenId, chainId) => {
      const mockedMetadata = mockNFTMetadata.find(
        (d) =>
          new Amount(d.tokenID).toHex() === new Amount(tokenId).toHex() &&
          d.contractInformation.address === contract
      )
      if (!mockedMetadata) {
        return {
          error: 1,
          errorMessage: 'metadata not found',
          tokenUrl: '',
          response: ''
        }
      }
      return {
        error: 0,
        errorMessage: '',
        tokenUrl: mockedMetadata.contractInformation.logo,
        response: JSON.stringify({
          attributes: [
            {
              trait_type: 'mocked trait name',
              value: '100%'
            } as { trait_type: string; value: string }
          ],
          description: mockedMetadata.contractInformation.description,
          image: mockedMetadata.imageURL,
          name: mockedMetadata.contractInformation.name
        } as CommonNftMetadata)
      }
    },
    getERC1155Metadata: async (contract, tokenId, chainId) => {
      return this.jsonRpcService.getERC721Metadata!(contract, tokenId, chainId)
    },
    getSolTokenMetadata: async (chainId, tokenMintAddress) => {
      const mockedMetadata =
        mockNFTMetadata.find(
          (d) =>
            d.tokenID === tokenMintAddress &&
            d.contractInformation.address === tokenMintAddress
        ) || mockNFTMetadata[0]
      return {
        error: 0,
        errorMessage: '',
        tokenUrl: mockedMetadata.contractInformation.logo,
        response: JSON.stringify({
          attributes: [
            {
              trait_type: 'mocked trait name',
              value: '100%'
            } as { trait_type: string; value: string }
          ],
          description: mockedMetadata.contractInformation.description,
          image: mockedMetadata.imageURL,
          name: mockedMetadata.contractInformation.name
        } as CommonNftMetadata)
      }
    },
    getNftMetadatas: async (coin, nftIdentifiers) => {
      const metadatas: BraveWallet.NftMetadata[] = nftIdentifiers.map((id) => {
        const mockedMetadata = mockNFTMetadata.find((d) => {
          return (
            d.contractInformation.address === id.contractAddress &&
            new Amount(d.tokenID).toHex() === new Amount(id.tokenId).toHex()
          )
        })

        if (!mockedMetadata) {
          throw new Error(
            `metadata not found for ${id.contractAddress}-${id.tokenId}`
          )
        }

        return {
          name: mockedMetadata.contractInformation.name,
          description: mockedMetadata.contractInformation.description,
          image: mockedMetadata.imageURL || '',
          externalUrl: '',
          attributes: [
            {
              traitType: 'mocked trait name',
              value: '100%'
            }
          ],
          imageData: '',
          backgroundColor: 'green',
          animationUrl: mockedMetadata.animationURL || '',
          youtubeUrl: 'youtube.com',
          collection: mockedMetadata.collection?.name || ''
        }
      })

      return {
        errorMessage: metadatas.length ? '' : 'metadata not found',
        metadatas
      }
    },
    // name service lookups
    setEnsOffchainLookupResolveMethod(method) {
      this.requireOffchainConsent = method
    },
    ensGetEthAddr: async (domain) => {
      return {
        address: `0x1234abcd1234${domain}`,
        error: 0,
        errorMessage: '',
        requireOffchainConsent:
          this.requireOffchainConsent !== BraveWallet.ResolveMethod.kEnabled
      }
    },
    snsGetSolAddr: async (domain) => {
      return {
        address: `s1abcd1234567890${domain}`,
        error: 0,
        errorMessage: ''
      }
    },
    unstoppableDomainsGetWalletAddr: async (domain, token) => {
      return {
        address: `0x${token?.chainId}abcd${domain}`,
        error: 0,
        errorMessage: ''
      }
    },

    getEthTokenInfo: async (contractAddress, chainId) => {
      // handle error case
      if (contractAddress === '0xInvalidToken') {
        return {
          token: null,
          error: BraveWallet.ProviderError.kResourceNotFound,
          errorMessage: 'token not found'
        }
      }

      const foundToken = mockTokensList.find(
        (t) => t.contractAddress === contractAddress
      )

      const metadata = mockNFTMetadata.find((meta) => {
        return meta.contractInformation.address === contractAddress
      })

      return {
        token: {
          contractAddress,
          chainId,
          coin: BraveWallet.CoinType.ETH,
          name:
            metadata?.collection?.name ||
            metadata?.contractInformation?.name ||
            foundToken?.name ||
            'Mocked Token',
          symbol: foundToken?.symbol || 'MTK',
          decimals: foundToken?.decimals || 18,
          coingeckoId: foundToken?.coingeckoId || 'mocked-token',
          isErc20: true,
          isErc721: false,
          isErc1155: false,
          splTokenProgram: BraveWallet.SPLTokenProgram.kUnsupported,
          isNft: false,
          isCompressed: false,
          tokenId: '',
          logo: '',
          isSpam: false,
          visible: false
        },
        error: 0,
        errorMessage: ''
      }
    },
    getERC721OwnerOf: async (contract, tokenId, chainId) => {
      if (contract === mockMoonCatNFT.contractAddress) {
        return { ownerAddress: mockAccount.address, error: 0, errorMessage: '' }
      }
      return {
        error: 0,
        errorMessage: '',
        ownerAddress: '0xDeadBeef'
      }
    }
  }

  solanaTxManagerProxy: Partial<
    InstanceType<typeof BraveWallet.SolanaTxManagerProxyInterface>
  > = {
    getSolanaTxFeeEstimation: async (chainId, txMetaId) => {
      return {
        error: 0,
        errorMessage: '',
        fee: {
          baseFee: BigInt(0),
          computeUnits: 0,
          feePerComputeUnit: BigInt(0)
        }
      }
    }
  }

  walletHandler: Partial<
    InstanceType<typeof BraveWallet.WalletHandlerInterface>
  > = {
    getWalletInfo: async (): Promise<{
      walletInfo: BraveWallet.WalletInfo
    }> => {
      return {
        walletInfo: {
          isBitcoinEnabled: true,
          isBitcoinImportEnabled: true,
          isZCashEnabled: true,
          isWalletBackedUp: true,
          isWalletCreated: true,
          isWalletLocked: false,
          isAnkrBalancesFeatureEnabled: false,
          isTransactionSimulationsFeatureEnabled: true
        }
      }
    }
  }

  txService: Partial<InstanceType<typeof BraveWallet.TxServiceInterface>> = {
    getAllTransactionInfo: async (coinType, chainId, from) => {
      // return all txs if filters are null
      if (coinType === null && chainId === null && from === null) {
        return {
          transactionInfos: this.transactionInfos
        }
      }

      const filteredTxs = this.transactionInfos.filter((tx) => {
        if (from && coinType !== null) {
          const txCoinType = getCoinFromTxDataUnion(tx.txDataUnion)

          return (
            // match from address + cointype
            txCoinType === coinType &&
            tx.fromAccountId.uniqueKey === from.uniqueKey &&
            // match chain id (if set)
            (chainId !== null ? tx.chainId === chainId : true)
          )
        }

        return (
          // match chain id
          chainId !== null ? tx.chainId === chainId : true
        )
      })

      return {
        transactionInfos: filteredTxs
      }
    },

    getTransactionInfo: async (
      coinType: number,
      chainId: string,
      txMetaId: string
    ) => {
      const foundTx = this.transactionInfos.find(
        (tx) =>
          getCoinFromTxDataUnion(tx.txDataUnion) === coinType &&
          tx.chainId === chainId &&
          tx.id === txMetaId
      )
      return {
        transactionInfo: foundTx || null
      }
    }
  }

  simulationService: Partial<
    InstanceType<typeof BraveWallet.SimulationServiceInterface>
  > = {
    hasMessageScanSupport: async (chainId, coin) => ({ result: false }),
    hasTransactionScanSupport: async () => ({ result: true }),
    scanEVMTransaction: async (txInfo, language) => {
      return {
        errorResponse: '',
        errorString: '',
        response: this
          .evmSimulationResponse as BraveWallet.EVMSimulationResponse
      }
    },
    scanSolanaTransaction: async (request, language) => {
      return {
        errorResponse: '',
        errorString: '',
        response: this
          .svmSimulationResponse as BraveWallet.SolanaSimulationResponse
      }
    }
  }

  braveWalletIpfsService: Partial<
    InstanceType<typeof BraveWallet.IpfsServiceInterface>
  > = {
    translateToGatewayURL: async function (url: string) {
      return {
        translatedUrl: url
      }
    }
  }

  setMockedQuote(newQuote: typeof this.mockZeroExQuote) {
    this.mockZeroExQuote = newQuote
  }

  setMockedTransactionPayload(newTx: typeof this.mockZeroExQuote) {
    this.mockZeroExTransaction = newTx
  }

  setMockedStore = (newStore: typeof this.store) => {
    this.store = newStore
  }
}

let apiProxy: Partial<WalletApiProxy> | undefined

type DappListResult =
  (typeof mockDappsListMap)[keyof typeof mockDappsListMap]['results'][number]

function createDappParserForRange(
  range: string
): (value: DappListResult) => BraveWallet.Dapp {
  return (d) => ({
    balance: d.metrics.balance ?? 0,
    categories: d.categories,
    chains: d.chains,
    description: d.description,
    id: d.dappId,
    logo: d.logo,
    name: d.name,
    range: range,
    transactions: d.metrics.transactions,
    uaw: d.metrics.uaw,
    volume: d.metrics.volume,
    website: d.website
  })
}

export function getAPIProxy(): Partial<WalletApiProxy> {
  if (!apiProxy) {
    apiProxy =
      new MockedWalletApiProxy() as unknown as Partial<WalletApiProxy> &
        MockedWalletApiProxy
  }
  return apiProxy
}

export function getMockedAPIProxy(): WalletApiProxy & MockedWalletApiProxy {
  return getAPIProxy() as unknown as WalletApiProxy & MockedWalletApiProxy
}

export function resetAPIProxy(overrides?: WalletApiDataOverrides | undefined) {
  apiProxy = new MockedWalletApiProxy(
    overrides
  ) as unknown as Partial<WalletApiProxy> & MockedWalletApiProxy
}

export default getAPIProxy
