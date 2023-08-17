// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assert } from 'chrome://resources/js/assert_ts.js'

// redux
import { createStore, combineReducers } from 'redux'
import { createWalletReducer } from '../../slices/wallet.slice'

// types
import {
  BraveWallet,
  SafeBlowfishEvmResponse,
  SafeBlowfishSolanaResponse,
  TxSimulationOptInStatus
} from '../../../constants/types'
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
  mockErc721Token,
  mockEthAccountInfo,
  mockFilecoinAccountInfo,
  mockFilecoinMainnetNetwork,
  mockSolanaAccountInfo,
  mockSolanaMainnetNetwork,
  mockSplNft
} from '../../constants/mocks'
import { mockEthMainnet, mockNetworks } from '../../../stories/mock-data/mock-networks'
import {
  mockAccountAssetOptions,
  mockBasicAttentionToken,
  mockErc20TokensList,
} from '../../../stories/mock-data/mock-asset-options'
import {
  mockFilSendTransaction,
  mockTransactionInfo,
  mockedErc20ApprovalTransaction
} from '../../../stories/mock-data/mock-transaction-info'
import { blockchainTokenEntityAdaptor } from '../../slices/entities/blockchain-token.entity'
import { findAccountByAccountId } from '../../../utils/account-utils'
import { CommonNftMetadata } from '../../slices/endpoints/nfts.endpoints'
import { mockNFTMetadata } from '../../../stories/mock-data/mock-nft-metadata'

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
  | Record<
      string, // chainId
      string // balance
    >
  | undefined
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
  selectedAccountId?: BraveWallet.AccountId
  chainIdsForCoins?: Record<BraveWallet.CoinType, string>
  networks?: BraveWallet.NetworkInfo[]
  defaultBaseCurrency?: string
  transactionInfos?: BraveWallet.TransactionInfo[]
  blockchainTokens?: BraveWallet.BlockchainToken[]
  userAssets?: BraveWallet.BlockchainToken[]
  accountInfos?: BraveWallet.AccountInfo[]
  nativeBalanceRegistry?: NativeAssetBalanceRegistry
  tokenBalanceRegistry?: TokenBalanceRegistry
  simulationOptInStatus?: TxSimulationOptInStatus
  evmSimulationResponse?: BraveWallet.EVMSimulationResponse
    | SafeBlowfishEvmResponse
    | null
  svmSimulationResponse?: BraveWallet.SolanaSimulationResponse
    | SafeBlowfishSolanaResponse
    | null
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
    mockSplNft
  ]

  userAssets: BraveWallet.BlockchainToken[] = mockAccountAssetOptions

  evmSimulationResponse: BraveWallet.EVMSimulationResponse
    | SafeBlowfishEvmResponse
    | null = null

  svmSimulationResponse: BraveWallet.SolanaSimulationResponse
    | SafeBlowfishSolanaResponse
    | null = null

  txSimulationOptInStatus: TxSimulationOptInStatus = 'allowed'

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
    }
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
    getAllAccounts: async (): Promise<{
      allAccounts: BraveWallet.AllAccountsInfo
    }> => {
      const selectedAccount = findAccountByAccountId(
        this.accountInfos,
        this.selectedAccountId
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
    // Native asset balances
    getBalance: async (address: string, coin: number, chainId: string) => {
      return {
        balance: this.nativeBalanceRegistry?.[address]?.[chainId] || '0',
        error: 0,
        errorMessage: ''
      }
    },
    getSolanaBalance: async (pubkey: string, chainId: string) => {
      return {
        balance: BigInt(this.nativeBalanceRegistry[pubkey]?.[chainId] || 0),
        error: 0,
        errorMessage: ''
      }
    },
    getERC20TokenBalance: async (contract, address, chainId) => {
      return {
        balance:
          this.nativeBalanceRegistry[address]?.[
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
          this.nativeBalanceRegistry[accountAddress]?.[
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
          this.nativeBalanceRegistry[accountAddress]?.[
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
          this.nativeBalanceRegistry[walletAddress]?.[
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
    getERC721Metadata: async (contract, tokenId, chainId) => {
      const mockedMetadata =
        mockNFTMetadata.find(
          (d) =>
            d.tokenID === tokenId && d.contractInformation.address === contract
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
    }
  }

  solanaTxManagerProxy: Partial<
    InstanceType<typeof BraveWallet.SolanaTxManagerProxyInterface>
  > = {
    getEstimatedTxFee: async (chainId, txMetaId) => {
      return {
        error: 0,
        errorMessage: '',
        fee: BigInt(100)
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
    extractIPFSUrlFromGatewayLikeUrl: async function (url: string) {
      return { ipfsUrl: url }
    },
    translateToNFTGatewayURL: async function (url: string) {
      return {
        translatedUrl: url
      }
    },
    translateToGatewayURL: async function (url: string) {
      return {
        translatedUrl: url
      }
    }
  }

  setMockedQuote(newQuote: typeof this.mockQuote) {
    this.mockQuote = newQuote
  }

  setMockedTransactionPayload(newTx: typeof this.mockQuote) {
    this.mockTransaction = newTx
  }

  setMockedStore = (newStore: typeof this.store) => {
    this.store = newStore
  }
}

let apiProxy: Partial<WalletApiProxy> | undefined

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
