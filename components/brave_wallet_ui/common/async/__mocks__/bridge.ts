// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// redux
import { createStore, combineReducers } from 'redux'
import { createWalletReducer } from '../../slices/wallet.slice'

// types
import { BraveWallet } from '../../../constants/types'
import { WalletActions } from '../../actions'
import type WalletApiProxy from '../../wallet_api_proxy'

// utils
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { deserializeTransaction } from '../../../utils/model-serialization-utils'

// mocks
import { mockWalletState } from '../../../stories/mock-data/mock-wallet-state'
import { mockedMnemonic } from '../../../stories/mock-data/user-accounts'
import {
  mockAccount,
  mockEthAccountInfo,
  mockFilecoinAccountInfo,
  mockFilecoinMainnetNetwork,
  mockSolanaAccountInfo,
  mockSolanaMainnetNetwork
} from '../../constants/mocks'
import { mockEthMainnet, mockNetworks } from '../../../stories/mock-data/mock-networks'
import {
  mockAccountAssetOptions,
  mockBasicAttentionToken,
  mockErc20TokensList
} from '../../../stories/mock-data/mock-asset-options'
import {
  mockFilSendTransaction,
  mockTransactionInfo,
  mockedErc20ApprovalTransaction
} from '../../../stories/mock-data/mock-transaction-info'
import { blockchainTokenEntityAdaptor } from '../../slices/entities/blockchain-token.entity'

export const makeMockedStoreWithSpy = () => {
  const store = createStore(combineReducers({
    wallet: createWalletReducer(mockWalletState)
  }))

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

type NativeAssetBalanceRegistry = Record<
  string, // account address
  Record<
    string, // chainId
    string // balance
  >
>

type TokenBalanceRegistry = Record<
  string, // account address
  Record<
    string, // asset identifier
    string // balance
  >
>

export interface WalletApiDataOverrides {
  selectedCoin?: BraveWallet.CoinType
  selectedAccountAddress?: string
  chainIdsForCoins?: Record<BraveWallet.CoinType, string>
  networks?: BraveWallet.NetworkInfo[]
  defaultBaseCurrency?: string
  transactionInfos?: BraveWallet.TransactionInfo[]
  blockchainTokens?: BraveWallet.BlockchainToken[]
  userAssets?: BraveWallet.BlockchainToken[]
  accountInfos?: BraveWallet.AccountInfo[]
  nativeBalanceRegistry?: NativeAssetBalanceRegistry
  tokenBalanceRegistry?: TokenBalanceRegistry
}

export class MockedWalletApiProxy {
  store = makeMockedStoreWithSpy().store

  defaultBaseCurrency: string = 'usd'
  selectedCoin: BraveWallet.CoinType = BraveWallet.CoinType.ETH
  selectedAccountAddress: string = mockAccount.address
  accountInfos: BraveWallet.AccountInfo[] = [
    mockEthAccountInfo,
    mockSolanaAccountInfo,
    mockFilecoinAccountInfo
  ]

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

  blockchainTokens: BraveWallet.BlockchainToken[] = mockErc20TokensList
  userAssets: BraveWallet.BlockchainToken[] = mockAccountAssetOptions

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

  mockQuote = {
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
    sources: []
  }

  mockTransaction = {
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
    sources: []
  }

  transactionInfos: BraveWallet.TransactionInfo[] = [
    deserializeTransaction(mockTransactionInfo),
    mockFilSendTransaction as BraveWallet.TransactionInfo,
    deserializeTransaction(mockedErc20ApprovalTransaction)
  ]

  constructor (overrides?: WalletApiDataOverrides | undefined) {
    this.applyOverrides(overrides)
  }

  applyOverrides (overrides?: WalletApiDataOverrides | undefined) {
    if (!overrides) {
      return
    }

    this.selectedAccountAddress =
      overrides.selectedAccountAddress ?? this.selectedAccountAddress
    this.selectedCoin = overrides.selectedCoin ?? this.selectedCoin
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
    getSelectedCoin: async () => {
      return { coin: this.selectedCoin }
    },
    setSelectedCoin: (coin) => {
      this.selectedCoin = coin
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
          origin: {
            scheme: 'https',
            host: 'brave.com',
            port: 443,
            nonceIfOpaque: undefined
          },
          originSpec: 'https://brave.com',
          eTldPlusOne: 'brave.com'
        }
      }
    },
  }

  swapService: Partial<InstanceType<typeof BraveWallet.SwapServiceInterface>> =
    {
      getTransactionPayload: async ({
        buyAmount,
        buyToken,
        sellAmount,
        sellToken
      }: BraveWallet.SwapParams): Promise<{
        response: BraveWallet.SwapResponse
        errorResponse: BraveWallet.SwapErrorResponse
        errorString: string
      }> => ({
        errorResponse: {
          code: 0,
          isInsufficientLiquidity: false,
          reason: '',
          validationErrors: []
        },
        response: {
          ...this.mockQuote,
          buyTokenAddress: buyToken,
          sellTokenAddress: sellToken,
          buyAmount: buyAmount || '',
          sellAmount: sellAmount || '',
          price: '1'
        },
        errorString: ''
      }),
      getPriceQuote: async () => ({
        response: this.mockTransaction,
        errorResponse: null,
        errorString: ''
      })
    }

  keyringService: Partial<
    InstanceType<typeof BraveWallet.KeyringServiceInterface>
  > = {
    getSelectedAccount: async () => {
      return { address: this.selectedAccountAddress }
    },
    validatePassword: async (password: string) => ({
      result: password === 'password'
    }),
    lock: () => {
      this.store.dispatch(WalletActions.locked())
      alert('wallet locked')
    },
    encodePrivateKeyForExport: async (
      address: string,
      password: string,
      coin: number
    ) =>
      password === 'password'
        ? { privateKey: 'secret-private-key' }
        : { privateKey: '' },
    getMnemonicForDefaultKeyring: async (password) => {
      return password === 'password'
        ? { mnemonic: mockedMnemonic }
        : { mnemonic: '' }
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

  braveWalletP3A: Partial<
    InstanceType<typeof BraveWallet.BraveWalletP3AInterface>
  > = {
    reportOnboardingAction: () => {},
    reportJSProvider: () => {}
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
    }
  }

  jsonRpcService: Partial<
    InstanceType<typeof BraveWallet.JsonRpcServiceInterface>
  > = {
    getAllNetworks: async (coin: BraveWallet.CoinType) => {
      return { networks: this.networks.filter((n) => n.coin === coin) }
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
    setNetwork: async (chainId, coin) => {
      this.selectedCoin = coin
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
    // Native asset balances
    getBalance: async (address: string, coin: number, chainId: string) => {
      return {
        balance: this.nativeBalanceRegistry[address][chainId],
        error: 0,
        errorMessage: ''
      }
    },
    getSolanaBalance: async (pubkey: string, chainId: string) => {
      return {
        balance: BigInt(this.nativeBalanceRegistry[pubkey][chainId]),
        error: 0,
        errorMessage: ''
      }
    },
    getERC20TokenBalance: async (contract, address, chainId) => {
      return {
        balance:
          this.nativeBalanceRegistry[address][
            blockchainTokenEntityAdaptor.selectId({
              chainId,
              contractAddress: contract,
              isErc721: false,
              tokenId: ''
            })
          ],
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
          this.nativeBalanceRegistry[accountAddress][
            blockchainTokenEntityAdaptor.selectId({
              chainId,
              contractAddress: contractAddress,
              isErc721: true,
              tokenId
            })
          ],
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
          this.nativeBalanceRegistry[accountAddress][
            blockchainTokenEntityAdaptor.selectId({
              chainId,
              contractAddress: contractAddress,
              isErc721: true,
              tokenId
            })
          ],
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
          this.nativeBalanceRegistry[walletAddress][
            blockchainTokenEntityAdaptor.selectId({
              chainId,
              contractAddress: tokenMintAddress,
              isErc721: true,
              tokenId: ''
            })
          ],
        decimals: 9,
        uiAmountString: '',
        error: 0,
        errorMessage: '',
      }
    }
  }

  solanaTxManagerProxy: Partial<InstanceType<typeof BraveWallet.SolanaTxManagerProxyInterface>> = {
    getEstimatedTxFee: async (chainId, txMetaId) => {
      return {
        error: 0,
        errorMessage: '',
        fee: BigInt(100)
      }
    },
  }

  walletHandler: Partial<
    InstanceType<typeof BraveWallet.WalletHandlerInterface>
  > = {
    getWalletInfo: async () => {
      return {
        walletInfo: {
          accountInfos: this.accountInfos,
          favoriteApps: [],
          isSolanaEnabled: true,
          isFilecoinEnabled: true,
          isBitcoinEnabled: true,
          isWalletBackedUp: true,
          isWalletCreated: true,
          isWalletLocked: false,
          isNftPinningFeatureEnabled: false,
          isPanelV2FeatureEnabled: false
        }
      }
    }
  }

  txService: Partial<InstanceType<typeof BraveWallet.TxServiceInterface>> = {
    getAllTransactionInfo: async (coinType, chainId, from) => {
      return {
        transactionInfos: this.transactionInfos.filter((tx) => {
          // return all txs if filters are null
          if (coinType === null && chainId === null && from === null) {
            return true
          }

          if (from && coinType !== null) {
            const txCoinType = getCoinFromTxDataUnion(tx.txDataUnion)

            return (
              // match from address + cointype
              txCoinType === coinType &&
              tx.fromAddress === from &&
              // match chain id (if set)
              (chainId !== null ? tx.chainId === chainId : true)
            )
          }

          return (
            // match chain id
            chainId !== null ? tx.chainId === chainId : true
          )
        })
      }
    }
  }

  setMockedQuote (newQuote: typeof this.mockQuote) {
    this.mockQuote = newQuote
  }

  setMockedTransactionPayload (newTx: typeof this.mockQuote) {
    this.mockTransaction = newTx
  }

  setMockedStore = (newStore: typeof this.store) => {
    this.store = newStore
  }
}

let apiProxy: Partial<WalletApiProxy>

export function getAPIProxy (): Partial<WalletApiProxy> {
  if (!apiProxy) {
    apiProxy =
      new MockedWalletApiProxy() as unknown as Partial<WalletApiProxy> &
        MockedWalletApiProxy
  }
  return apiProxy
}

export function getMockedAPIProxy (): WalletApiProxy & MockedWalletApiProxy {
  return getAPIProxy() as unknown as WalletApiProxy & MockedWalletApiProxy
}

export default getAPIProxy
