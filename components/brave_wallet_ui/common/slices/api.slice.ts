// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { batch } from 'react-redux'
import { Store } from '@reduxjs/toolkit'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { PatchCollection } from '@reduxjs/toolkit/dist/query/core/buildThunks'
import { mapLimit } from 'async'

// types
import { WalletPanelApiProxy } from '../../panel/wallet_panel_api_proxy'
import {
  ApproveERC20Params,
  BraveWallet,
  ER20TransferParams,
  ERC721TransferFromParams,
  ETHFilForwarderTransferFromParams,
  SendBtcTransactionParams,
  SendEthTransactionParams,
  SendFilTransactionParams,
  SendSolTransactionParams,
  SendZecTransactionParams,
  SerializableTransactionInfo,
  SPLTransferFromParams
} from '../../constants/types'
import {
  CancelTransactionPayload,
  RetryTransactionPayload,
  SpeedupTransactionPayload,
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType,
  UpdateUnapprovedTransactionSpendAllowanceType
} from '../constants/action_types'
import { PanelActions } from '../../panel/actions'

// entities
import {
  networkEntityAdapter,
  selectMainnetNetworksFromQueryResult,
  selectAllNetworksFromQueryResult,
  selectOffRampNetworksFromQueryResult,
  selectOnRampNetworksFromQueryResult,
  selectVisibleNetworksFromQueryResult
} from './entities/network.entity'

// api
import { apiProxyFetcher } from '../async/base-query-cache'
import { createWalletApiBase } from './api-base.slice'
import { transactionSimulationEndpoints } from './endpoints/tx-simulation.endpoints'
import { braveRewardsApiEndpoints } from './endpoints/rewards.endpoints'
import { p3aEndpoints } from './endpoints/p3a.endpoints'
import { pricingEndpoints } from './endpoints/pricing.endpoints'
import { nftsEndpoints } from './endpoints/nfts.endpoints'
import { qrCodeEndpoints } from './endpoints/qr-code.endpoints'

// utils
import { handleEndpointError } from '../../utils/api-utils'
import {
  findAccountByAccountId,
  getAccountType
} from '../../utils/account-utils'
import { TX_CACHE_TAGS } from '../../utils/query-cache-utils'
import {
  getCoinFromTxDataUnion,
  hasEIP1559Support
} from '../../utils/network-utils'
import {
  shouldReportTransactionP3A,
  sortTransactionByDate,
  toTxDataUnion
} from '../../utils/tx-utils'
import { makeSerializableTransaction } from '../../utils/model-serialization-utils'
import {
  dialogErrorFromLedgerErrorCode,
  signLedgerEthereumTransaction,
  signLedgerFilecoinTransaction,
  signLedgerSolanaTransaction,
  signTrezorTransaction
} from '../async/hardware'
import { walletEndpoints } from './endpoints/wallet.endpoints'
import { tokenEndpoints } from './endpoints/token.endpoints'
import { onRampEndpoints } from './endpoints/on-ramp.endpoints'
import { offRampEndpoints } from './endpoints/off-ramp.endpoints'
import { coingeckoEndpoints } from './endpoints/coingecko-endpoints'
import {
  tokenSuggestionsEndpoints //
} from './endpoints/token_suggestions.endpoints'
import { addressEndpoints } from './endpoints/address.endpoints'
import { accountEndpoints } from './endpoints/account.endpoints'
import { networkEndpoints } from './endpoints/network.endpoints'
import { coinMarketEndpoints } from './endpoints/market.endpoints'
import { tokenBalancesEndpoints } from './endpoints/token_balances.endpoints'
import { fiatCurrencyEndpoints } from './endpoints/fiat_currency.endpoints'
import { sitePermissionEndpoints } from './endpoints/site_permissions.endpoints'

interface GetTransactionsQueryArg {
  /**
   * will fetch for all account addresses if null
   */
  coinType: BraveWallet.CoinType | null
  /**
   * will fetch for all coin-type addresses if null
   */
  accountId: BraveWallet.AccountId | null
  /**
   * will fetch for all coin-type chains if null
   */
  chainId: string | null
}

export function createWalletApi() {
  // base to add endpoints to
  return (
    createWalletApiBase()
      .injectEndpoints({
        endpoints: ({ mutation, query }) => {
          return {
            //
            // Transactions
            //
            invalidateTransactionsCache: mutation<boolean, void>({
              queryFn: () => {
                return { data: true }
              }, // no-op, uses invalidateTags
              invalidatesTags: ['Transactions']
            }),
            getTransactions: query<
              SerializableTransactionInfo[],
              GetTransactionsQueryArg
            >({
              queryFn: async (
                { accountId: fromAccountId, coinType, chainId },
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const {
                    data: { txService },
                    cache
                  } = baseQuery(undefined)
                  // TODO(apaymyshev): getAllTransactionInfo already supports
                  // getting transaction for all accounts.
                  const txInfos =
                    fromAccountId && coinType !== null
                      ? (
                          await txService.getAllTransactionInfo(
                            coinType,
                            chainId,
                            fromAccountId
                          )
                        ).transactionInfos
                      : (
                          await mapLimit(
                            (
                              await cache.getAllAccounts()
                            ).accounts,
                            10,
                            async (account: BraveWallet.AccountInfo) => {
                              const { transactionInfos } =
                                await txService.getAllTransactionInfo(
                                  account.accountId.coin,
                                  chainId,
                                  account.accountId
                                )
                              return transactionInfos
                            }
                          )
                        ).flat(1)

                  const nonRejectedTransactionInfos = txInfos
                    // hide rejected txs
                    .filter(
                      (tx) =>
                        tx.txStatus !== BraveWallet.TransactionStatus.Rejected
                    )
                    .map(makeSerializableTransaction)

                  const sortedTxs = sortTransactionByDate(
                    nonRejectedTransactionInfos,
                    'descending'
                  )

                  return {
                    data: sortedTxs
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to fetch txs for account: ${
                      fromAccountId?.uniqueKey //
                    } (${
                      coinType //
                    })`,
                    error
                  )
                }
              },
              providesTags: (res, err, arg) =>
                err
                  ? ['UNKNOWN_ERROR']
                  : [
                      ...TX_CACHE_TAGS.LISTS({
                        chainId: arg.chainId,
                        coin: arg.coinType,
                        fromAccountId: arg.accountId
                      }),
                      ...TX_CACHE_TAGS.IDS((res || []).map(({ id }) => id))
                    ]
            }),
            sendEthTransaction: mutation<
              { success: boolean },
              SendEthTransactionParams
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data
                  /***
                   * Determine whether to create a legacy or EIP-1559
                   * transaction.
                   *
                   * isEIP1559 is true IFF:
                   *   - network supports EIP-1559
                   *
                   *     AND
                   *
                   *   - keyring supports EIP-1559
                   *     (ex: certain hardware wallets vendors)
                   *
                   *     AND
                   *
                   *   - payload: SendEthTransactionParams has NOT specified
                   *     legacy gas-pricing fields.
                   *
                   * In all other cases, fallback to legacy gas-pricing fields.
                   * For example, if network and keyring support EIP-1559, but
                   * the legacy gasPrice field is specified in the payload, then
                   * type-0 transaction should be created.
                   */
                  const isEIP1559 =
                    payload.gasPrice === undefined &&
                    hasEIP1559Support(
                      getAccountType(payload.fromAccount),
                      payload.network
                    )

                  const txData: BraveWallet.TxData = {
                    nonce: '',
                    // Estimated by eth_tx_service
                    // if value is '' for legacy transactions
                    gasPrice: isEIP1559 ? '' : payload.gasPrice || '',
                    // Estimated by eth_tx_service if value is ''
                    gasLimit: payload.gas || '',
                    to: payload.to,
                    value: payload.value,
                    data: payload.data || [],
                    signOnly: false,
                    signedTransaction: ''
                  }

                  const txData1559: BraveWallet.TxData1559 = {
                    baseData: txData,
                    chainId: payload.network.chainId,
                    // Estimated by eth_tx_service if value is ''
                    maxPriorityFeePerGas: payload.maxPriorityFeePerGas || '',
                    // Estimated by eth_tx_service if value is ''
                    maxFeePerGas: payload.maxFeePerGas || '',
                    gasEstimation: undefined
                  }

                  const { errorMessage, success } =
                    await txService.addUnapprovedTransaction(
                      isEIP1559
                        ? toTxDataUnion({ ethTxData1559: txData1559 })
                        : toTxDataUnion({ ethTxData: txData }),
                      payload.network.chainId,
                      payload.fromAccount.accountId
                    )

                  if (!success && errorMessage) {
                    return {
                      error: `Failed to create Eth transaction: ${
                        errorMessage || 'unknown error'
                      } ::payload:: ${JSON.stringify(payload)}`
                    }
                  }

                  return {
                    data: { success }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Failed to create Eth transaction: ${
                      error || 'unknown error'
                    } ::payload:: ${JSON.stringify(payload)}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? []
                  : TX_CACHE_TAGS.LISTS({
                      coin: arg.fromAccount.accountId.coin,
                      fromAccountId: arg.fromAccount.accountId,
                      chainId: null
                    })
            }),
            sendFilTransaction: mutation<
              { success: boolean },
              SendFilTransactionParams
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data

                  const filTxData: BraveWallet.FilTxData = {
                    nonce: payload.nonce || '',
                    gasPremium: payload.gasPremium || '',
                    gasFeeCap: payload.gasFeeCap || '',
                    gasLimit: payload.gasLimit || '',
                    maxFee: payload.maxFee || '0',
                    to: payload.to,
                    value: payload.value
                  }

                  const { errorMessage, success } =
                    await txService.addUnapprovedTransaction(
                      toTxDataUnion({ filTxData: filTxData }),
                      payload.network.chainId,
                      payload.fromAccount.accountId
                    )

                  if (!success && errorMessage) {
                    return {
                      error: `Failed to send Fil transaction: ${
                        errorMessage || 'unknown error'
                      }`
                    }
                  }

                  return {
                    data: { success }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to send Fil transaction',
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? []
                  : TX_CACHE_TAGS.LISTS({
                      coin: arg.fromAccount.accountId.coin,
                      fromAccountId: arg.fromAccount.accountId,
                      chainId: null
                    })
            }),
            sendSolTransaction: mutation<
              { success: boolean },
              SendSolTransactionParams
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { solanaTxManagerProxy, txService } =
                    baseQuery(undefined).data

                  const {
                    error,
                    errorMessage: transferTxDataErrorMessage,
                    txData
                  } = await solanaTxManagerProxy //
                    .makeSystemProgramTransferTxData(
                      payload.fromAccount.address,
                      payload.to,
                      BigInt(payload.value)
                    )

                  if (error && transferTxDataErrorMessage) {
                    return {
                      error:
                        'Failed to make SOL system program transfer txData): ' +
                          transferTxDataErrorMessage || 'unknown error'
                    }
                  }

                  const { errorMessage, success } =
                    await txService.addUnapprovedTransaction(
                      toTxDataUnion({ solanaTxData: txData ?? undefined }),
                      payload.network.chainId,
                      payload.fromAccount.accountId
                    )

                  if (!success && errorMessage) {
                    return {
                      error: `Failed to send Sol transaction: ${
                        errorMessage || 'unknown error'
                      }`
                    }
                  }

                  return {
                    data: { success }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to send Sol transaction',
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? []
                  : TX_CACHE_TAGS.LISTS({
                      coin: arg.fromAccount.accountId.coin,
                      fromAccountId: arg.fromAccount.accountId,
                      chainId: null
                    })
            }),
            sendBtcTransaction: mutation<
              { success: boolean },
              SendBtcTransactionParams
            >({
              queryFn: async (
                payload,
                { dispatch },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data

                  const btcTxData: BraveWallet.BtcTxData = {
                    to: payload.to,
                    amount: BigInt(payload.value),
                    fee: BigInt(0),
                    inputs: [],
                    outputs: []
                  }

                  const { errorMessage, success } =
                    await txService.addUnapprovedTransaction(
                      toTxDataUnion({ btcTxData }),
                      payload.network.chainId,
                      payload.fromAccount.accountId
                    )

                  if (!success && errorMessage) {
                    return {
                      error: `Failed to send Btc transaction: ${
                        errorMessage || 'unknown error'
                      }`
                    }
                  }

                  return {
                    data: { success }
                  }
                } catch (error) {
                  return { error: 'Failed to send Btc transaction' }
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? []
                  : TX_CACHE_TAGS.LISTS({
                      coin: arg.fromAccount.accountId.coin,
                      fromAccountId: arg.fromAccount.accountId,
                      chainId: null
                    })
            }),
            sendZecTransaction: mutation<
              { success: boolean },
              SendZecTransactionParams
            >({
              queryFn: async (
                payload,
                { dispatch },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data

                  const zecTxData: BraveWallet.ZecTxData = {
                    to: payload.to,
                    amount: BigInt(payload.value),
                    fee: BigInt(0),
                    inputs: [],
                    outputs: []
                  }

                  const { errorMessage, success } =
                    await txService.addUnapprovedTransaction(
                      toTxDataUnion({ zecTxData }),
                      payload.network.chainId,
                      payload.fromAccount.accountId
                    )

                  if (!success && errorMessage) {
                    return {
                      error: `Failed to send Zec transaction: ${
                        errorMessage || 'unknown error'
                      }`
                    }
                  }

                  return {
                    data: { success }
                  }
                } catch (error) {
                  return { error: 'Failed to send Zec transaction' }
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? []
                  : TX_CACHE_TAGS.LISTS({
                      coin: arg.fromAccount.accountId.coin,
                      fromAccountId: arg.fromAccount.accountId,
                      chainId: null
                    })
            }),
            sendTransaction: mutation<
              { success: boolean },
              | SendEthTransactionParams
              | SendFilTransactionParams
              | SendSolTransactionParams
              | SendBtcTransactionParams
              | SendZecTransactionParams
            >({
              queryFn: async (
                payload,
                { dispatch, endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const coin = payload.fromAccount.accountId.coin
                  switch (coin) {
                    case BraveWallet.CoinType.SOL: {
                      const result: { success: boolean } = await dispatch(
                        walletApi.endpoints.sendSolTransaction.initiate(
                          payload as SendSolTransactionParams
                        )
                      ).unwrap()
                      return {
                        data: result
                      }
                    }
                    case BraveWallet.CoinType.FIL: {
                      const result: { success: boolean } = await dispatch(
                        walletApi.endpoints.sendFilTransaction.initiate(
                          payload as SendFilTransactionParams
                        )
                      ).unwrap()
                      return {
                        data: result
                      }
                    }
                    case BraveWallet.CoinType.ETH: {
                      const result: { success: boolean } = await dispatch(
                        walletApi.endpoints.sendEthTransaction.initiate(
                          payload as SendEthTransactionParams
                        )
                      ).unwrap()
                      return {
                        data: result
                      }
                    }
                    case BraveWallet.CoinType.BTC: {
                      const result: { success: boolean } = await dispatch(
                        walletApi.endpoints.sendBtcTransaction.initiate(
                          payload as SendBtcTransactionParams
                        )
                      ).unwrap()
                      return {
                        data: result
                      }
                    }
                    case BraveWallet.CoinType.ZEC: {
                      const result: { success: boolean } = await dispatch(
                        walletApi.endpoints.sendZecTransaction.initiate(
                          payload as SendZecTransactionParams
                        )
                      ).unwrap()
                      return {
                        data: result
                      }
                    }
                    default: {
                      return {
                        error: `Unsupported coin type" ${coin}`
                      }
                    }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Sending unapproved transaction failed:
                from=${payload.fromAccount.address}
              `,
                    error
                  )
                }
              }
              // invalidatesTags: handled by other 'send-X-Transaction` methods
            }),
            sendERC20Transfer: mutation<
              { success: boolean },
              ER20TransferParams
            >({
              queryFn: async (
                payload,
                { dispatch, endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data
                  const { data, success } =
                    await ethTxManagerProxy.makeERC20TransferData(
                      payload.to,
                      payload.value
                    )
                  if (!success) {
                    throw new Error(
                      `Failed making ERC20 transfer data, to: ${
                        payload.to //
                      }, value: ${
                        payload.value //
                      }`
                    )
                  }

                  const result: { success: boolean } = await dispatch(
                    walletApi.endpoints.sendTransaction.initiate({
                      network: payload.network,
                      fromAccount: payload.fromAccount,
                      to: payload.contractAddress,
                      value: '0x0',
                      gas: payload.gas,
                      gasPrice: payload.gasPrice,
                      maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                      maxFeePerGas: payload.maxFeePerGas,
                      data
                    })
                  ).unwrap()

                  return {
                    data: result
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to send ERC20 Transfer',
                    error
                  )
                }
              }
              // invalidatesTags: handled by other 'send-X-Transaction` methods
            }),
            sendSPLTransfer: mutation<
              { success: boolean },
              SPLTransferFromParams
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { solanaTxManagerProxy, txService } =
                    baseQuery(undefined).data

                  const { errorMessage: transferTxDataErrorMessage, txData } =
                    await solanaTxManagerProxy.makeTokenProgramTransferTxData(
                      payload.network.chainId,
                      payload.splTokenMintAddress,
                      payload.fromAccount.address,
                      payload.to,
                      BigInt(payload.value)
                    )

                  if (!txData) {
                    const errorMsg = `Failed making SPL transfer data
                to: ${payload.to}
                value: ${payload.value}
                error: ${transferTxDataErrorMessage}`
                    console.error(errorMsg)
                    return {
                      error: errorMsg
                    }
                  }

                  const { errorMessage, success } =
                    await txService.addUnapprovedTransaction(
                      toTxDataUnion({ solanaTxData: txData }),
                      payload.network.chainId,
                      payload.fromAccount.accountId
                    )

                  if (!success) {
                    const errorMsg =
                      'Unable to send SPL Transfer: ' + errorMessage
                    console.error(errorMessage)
                    return {
                      error: errorMsg
                    }
                  }

                  return {
                    data: {
                      success
                    }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `SPL Transfer failed:
                  to: ${payload.to}
                  value: ${payload.value}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                TX_CACHE_TAGS.LISTS({
                  chainId: null,
                  coin: arg.fromAccount.accountId.coin,
                  fromAccountId: arg.fromAccount.accountId
                })
            }),
            sendERC721TransferFrom: mutation<
              { success: boolean },
              ERC721TransferFromParams
            >({
              queryFn: async (
                payload,
                { dispatch, endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data
                  const { data, success } =
                    await ethTxManagerProxy.makeERC721TransferFromData(
                      payload.fromAccount.address,
                      payload.to,
                      payload.tokenId,
                      payload.contractAddress
                    )

                  if (!success) {
                    throw new Error(`Failed making ERC721 transferFrom data,
                from: ${payload.fromAccount.address}
                to: ${payload.to},
                tokenId: ${payload.tokenId}
              `)
                  }

                  const result: { success: boolean } = await dispatch(
                    walletApi.endpoints.sendTransaction.initiate({
                      network: payload.network,
                      fromAccount: payload.fromAccount,
                      to: payload.contractAddress,
                      value: '0x0',
                      gas: payload.gas,
                      gasPrice: payload.gasPrice,
                      maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                      maxFeePerGas: payload.maxFeePerGas,
                      data
                    })
                  ).unwrap()

                  return {
                    data: result
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to send ERC721 Transfer',
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                TX_CACHE_TAGS.LISTS({
                  chainId: null,
                  coin: arg.fromAccount.accountId.coin,
                  fromAccountId: arg.fromAccount.accountId
                })
            }),
            sendETHFilForwarderTransfer: mutation<
              { success: boolean },
              ETHFilForwarderTransferFromParams
            >({
              queryFn: async (
                payload,
                { dispatch, endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data
                  const { data, success } =
                    await ethTxManagerProxy.makeFilForwarderTransferData(
                      payload.to
                    )

                  if (!success) {
                    throw new Error(
                      'Failed making FilForwarder transferFrom data, from: ' +
                        payload.fromAccount.address +
                        ' to: ' +
                        payload.to
                    )
                  }

                  const result: { success: boolean } = await dispatch(
                    walletApi.endpoints.sendTransaction.initiate({
                      network: payload.network,
                      fromAccount: payload.fromAccount,
                      to: payload.contractAddress,
                      value: payload.value,
                      gas: payload.gas,
                      gasPrice: payload.gasPrice,
                      maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
                      maxFeePerGas: payload.maxFeePerGas,
                      data
                    })
                  ).unwrap()

                  return {
                    data: result
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to send ETH-Fil-Forwarder Transfer',
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                TX_CACHE_TAGS.LISTS({
                  chainId: null,
                  coin: arg.fromAccount.accountId.coin,
                  fromAccountId: arg.fromAccount.accountId
                })
            }),

            approveERC20Allowance: mutation<
              { success: boolean },
              ApproveERC20Params
            >({
              queryFn: async (
                payload,
                { dispatch, endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data

                  const { data, success } =
                    await ethTxManagerProxy.makeERC20ApproveData(
                      payload.spenderAddress,
                      payload.allowance
                    )

                  if (!success) {
                    throw new Error(`Failed making ERC20 approve data
                  contract: ${payload.contractAddress}
                  spender: ${payload.spenderAddress}
                  allowance: ${payload.allowance}
                `)
                  }

                  const result: { success: boolean } = await dispatch(
                    walletApi.endpoints.sendTransaction.initiate({
                      network: payload.network,
                      fromAccount: payload.fromAccount,
                      to: payload.contractAddress,
                      value: '0x0',
                      data
                    })
                  ).unwrap()

                  return { data: result }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to send ETH-Fil-Forwarder Transfer',
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                TX_CACHE_TAGS.LISTS({
                  chainId: null,
                  coin: BraveWallet.CoinType.ETH,
                  fromAccountId: arg.fromAccount.accountId
                })
            }),
            transactionStatusChanged: mutation<
              {
                success: boolean
                txId: string
                status: BraveWallet.TransactionStatus
              },
              Pick<
                SerializableTransactionInfo,
                'txStatus' | 'id' | 'chainId' | 'fromAccountId'
              > & {
                coinType: BraveWallet.CoinType
              }
            >({
              queryFn: async (arg) => {
                // no-op
                // uses 'invalidateTags' to handle data refresh
                return {
                  data: { success: true, txId: arg.id, status: arg.txStatus }
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? ['UNKNOWN_ERROR']
                  : [
                      TX_CACHE_TAGS.ID(arg.id),
                      ...(arg.txStatus ===
                      BraveWallet.TransactionStatus.Confirmed
                        ? ([
                            'UserBlockchainTokens', // refresh all user tokens
                            'AccountTokenCurrentBalance',
                            'TokenSpotPrices'
                          ] as const)
                        : [])
                    ],
              onQueryStarted: async (arg, { dispatch, queryFulfilled }) => {
                const txQueryArgsToUpdate: GetTransactionsQueryArg[] = [
                  {
                    accountId: arg.fromAccountId,
                    coinType: arg.coinType,
                    chainId: arg.chainId
                  },
                  {
                    accountId: arg.fromAccountId,
                    coinType: arg.coinType,
                    chainId: null
                  },
                  {
                    accountId: null,
                    coinType: arg.coinType,
                    chainId: arg.chainId
                  },
                  {
                    accountId: null,
                    coinType: null,
                    chainId: null
                  }
                ]

                const patchActions = txQueryArgsToUpdate.map((argsToUpdate) =>
                  walletApi.util.updateQueryData(
                    'getTransactions',
                    argsToUpdate,
                    (draft) => {
                      const foundTx = draft.find((tx) => tx.id === arg.id)
                      if (foundTx) {
                        foundTx.txStatus = arg.txStatus
                      }
                    }
                  )
                )

                const patchResults: PatchCollection[] = []
                // Note: batching not needed if we can upgrade to react 18+
                batch(() => {
                  for (const action of patchActions) {
                    const patch = dispatch(action)
                    patchResults.push(patch)
                  }
                })

                try {
                  await queryFulfilled
                } catch (error) {
                  patchResults.forEach((patchResult) => {
                    patchResult.undo()
                  })
                }
              }
            }),
            approveTransaction: mutation<
              { success: boolean },
              Pick<SerializableTransactionInfo, 'id' | 'chainId' | 'txType'> & {
                coinType: BraveWallet.CoinType
              }
            >({
              queryFn: async (
                txInfo,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const api = baseQuery(undefined).data
                  const { txService, braveWalletP3A } = api
                  const result: {
                    status: boolean
                    errorUnion: BraveWallet.ProviderErrorUnion
                    errorMessage: string
                  } = await txService.approveTransaction(
                    txInfo.coinType,
                    txInfo.chainId,
                    txInfo.id
                  )

                  const error =
                    result.errorUnion.providerError ??
                    result.errorUnion.solanaProviderError

                  if (error && error !== BraveWallet.ProviderError.kSuccess) {
                    return {
                      error: `${error}: ${result.errorMessage}`
                    }
                  }

                  if (shouldReportTransactionP3A({ txInfo })) {
                    braveWalletP3A.reportTransactionSent(txInfo.coinType, true)
                  }

                  return {
                    data: { success: true }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to approve transaction: ${error}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? ['UNKNOWN_ERROR'] : [TX_CACHE_TAGS.ID(arg.id)]
            }),
            approveHardwareTransaction: mutation<
              { success: boolean },
              Pick<
                SerializableTransactionInfo,
                'id' | 'txDataUnion' | 'txType' | 'fromAccountId' | 'chainId'
              >
            >({
              queryFn: async (txInfo, store, extraOptions, baseQuery) => {
                try {
                  const { data, cache } = baseQuery(undefined)
                  const apiProxy = data

                  const accountsRegistry = await cache.getAccountsRegistry()
                  const foundAccount = findAccountByAccountId(
                    txInfo.fromAccountId,
                    accountsRegistry
                  )

                  if (!foundAccount?.hardware) {
                    return {
                      error:
                        'failed to approve hardware transaction - ' +
                        'account not found or is not hardware: ' +
                        txInfo.fromAccountId.uniqueKey
                    }
                  }

                  const hardwareAccount: BraveWallet.HardwareInfo =
                    foundAccount.hardware

                  if (apiProxy instanceof WalletPanelApiProxy) {
                    navigateToConnectHardwareWallet(apiProxy, store)
                  }

                  if (
                    hardwareAccount.vendor ===
                    BraveWallet.LEDGER_HARDWARE_VENDOR
                  ) {
                    let success, error, code
                    switch (foundAccount.accountId.coin) {
                      case BraveWallet.CoinType.ETH:
                        ;({ success, error, code } =
                          await signLedgerEthereumTransaction(
                            apiProxy,
                            hardwareAccount.path,
                            txInfo,
                            foundAccount.accountId.coin
                          ))
                        break
                      case BraveWallet.CoinType.FIL:
                        ;({ success, error, code } =
                          await signLedgerFilecoinTransaction(
                            apiProxy,
                            txInfo,
                            foundAccount.accountId.coin
                          ))
                        break
                      case BraveWallet.CoinType.SOL:
                        ;({ success, error, code } =
                          await signLedgerSolanaTransaction(
                            apiProxy,
                            hardwareAccount.path,
                            txInfo,
                            foundAccount.accountId.coin
                          ))
                        break
                      default:
                        await store.dispatch(PanelActions.navigateToMain())
                        return {
                          error: `unsupported coin type for hardware approval`
                        }
                    }
                    if (success) {
                      store.dispatch(
                        PanelActions.setSelectedTransactionId(txInfo.id)
                      )
                      store.dispatch(
                        PanelActions.navigateTo('transactionDetails')
                      )
                      apiProxy.panelHandler?.setCloseOnDeactivate(true)
                      return {
                        data: { success: true }
                      }
                    }

                    if (code !== undefined) {
                      if (code === 'unauthorized') {
                        store.dispatch(
                          PanelActions.setHardwareWalletInteractionError(code)
                        )
                        return {
                          error: code
                        }
                      }

                      const deviceError = dialogErrorFromLedgerErrorCode(code)
                      if (deviceError === 'transactionRejected') {
                        await store.dispatch(
                          walletApi.endpoints.rejectTransaction.initiate({
                            chainId: txInfo.chainId,
                            coinType: getCoinFromTxDataUnion(
                              txInfo.txDataUnion
                            ),
                            id: txInfo.id
                          })
                        )
                        store.dispatch(PanelActions.navigateToMain())
                        return {
                          data: { success: true }
                        }
                      }

                      store.dispatch(
                        PanelActions.setHardwareWalletInteractionError(
                          deviceError
                        )
                      )
                      return {
                        error: deviceError
                      }
                    }

                    if (error) {
                      // TODO: handle non-device errors
                      console.log(error)
                      store.dispatch(PanelActions.navigateToMain())

                      return {
                        error:
                          typeof error === 'object'
                            ? JSON.stringify(error)
                            : error || 'unknown error'
                      }
                    }
                  } else if (
                    hardwareAccount.vendor ===
                    BraveWallet.TREZOR_HARDWARE_VENDOR
                  ) {
                    const { success, error, deviceError } =
                      await signTrezorTransaction(
                        apiProxy,
                        hardwareAccount.path,
                        txInfo
                      )
                    if (success) {
                      store.dispatch(
                        PanelActions.setSelectedTransactionId(txInfo.id)
                      )
                      store.dispatch(
                        PanelActions.navigateTo('transactionDetails')
                      )
                      apiProxy.panelHandler?.setCloseOnDeactivate(true)
                      // By default the focus is moved to the browser window
                      // automatically when Trezor popup closed which triggers
                      // an OnDeactivate event that would close the wallet panel
                      // because of the above API call. However, there could be
                      // times that the above call happens after OnDeactivate
                      // event, so the wallet panel would stay open after Trezor
                      // popup closed. As a workaround, we manually set the
                      // focus back to wallet panel here so it would trigger
                      // another OnDeactivate event when user clicks outside of
                      // the wallet panel.
                      apiProxy.panelHandler?.focus()
                      return {
                        data: { success: true }
                      }
                    }

                    if (deviceError === 'deviceBusy') {
                      // do nothing as the operation is already in progress
                      return {
                        data: { success: true }
                      }
                    }

                    console.log(error)
                    store.dispatch(
                      walletApi.endpoints.rejectTransaction.initiate({
                        chainId: txInfo.chainId,
                        coinType: getCoinFromTxDataUnion(txInfo.txDataUnion),
                        id: txInfo.id
                      })
                    )
                    store.dispatch(PanelActions.navigateToMain())
                    return {
                      data: { success: false }
                    }
                  }

                  store.dispatch(PanelActions.navigateToMain())

                  return {
                    data: { success: true }
                  }
                } catch (error) {
                  return handleEndpointError(
                    store.endpoint,
                    `Unable to approve hardware transaction: ${error}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? ['UNKNOWN_ERROR'] : [TX_CACHE_TAGS.ID(arg.id)]
            }),
            rejectTransaction: mutation<
              { success: boolean },
              Pick<BraveWallet.TransactionInfo, 'id' | 'chainId'> & {
                coinType: BraveWallet.CoinType
              }
            >({
              queryFn: async (tx, { endpoint }, extraOptions, baseQuery) => {
                try {
                  const { txService } = baseQuery(undefined).data
                  await txService.rejectTransaction(
                    tx.coinType,
                    tx.chainId,
                    tx.id
                  )
                  return {
                    data: { success: true }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Unable to reject transaction',
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.id)]
            }),
            rejectAllTransactions: mutation<{ success: boolean }, void>({
              queryFn: async (_arg, { dispatch, endpoint }) => {
                try {
                  const pendingTxs: SerializableTransactionInfo[] = (
                    await dispatch(
                      walletApi.endpoints.getTransactions.initiate({
                        chainId: null,
                        accountId: null,
                        coinType: null
                      })
                    ).unwrap()
                  ).filter(
                    (tx) =>
                      tx.txStatus === BraveWallet.TransactionStatus.Unapproved
                  )

                  await mapLimit(
                    pendingTxs,
                    10,
                    async (tx: SerializableTransactionInfo) => {
                      const { success } = await dispatch(
                        walletApi.endpoints.rejectTransaction.initiate({
                          coinType: getCoinFromTxDataUnion(tx.txDataUnion),
                          chainId: tx.chainId,
                          id: tx.id
                        })
                      ).unwrap()
                      return success
                    }
                  )

                  return {
                    data: { success: true }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Unable to reject all transactions',
                    error
                  )
                }
              }
              // invalidatesTags handled by rejectTransaction
            }),
            updateUnapprovedTransactionGasFields: mutation<
              { success: boolean },
              UpdateUnapprovedTransactionGasFieldsType
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data

                  const isEIP1559 =
                    payload.maxPriorityFeePerGas !== undefined &&
                    payload.maxFeePerGas !== undefined

                  if (isEIP1559) {
                    const result = await ethTxManagerProxy //
                      .setGasFeeAndLimitForUnapprovedTransaction(
                        payload.chainId,
                        payload.txMetaId,
                        payload.maxPriorityFeePerGas || '',
                        payload.maxFeePerGas || '',
                        payload.gasLimit
                      )

                    if (!result.success) {
                      throw new Error(
                        'Failed to update unapproved transaction: ' +
                          `id=${payload.txMetaId} ` +
                          'maxPriorityFeePerGas=' +
                          payload.maxPriorityFeePerGas +
                          `maxFeePerGas=${payload.maxFeePerGas}` +
                          `gasLimit=${payload.gasLimit}`
                      )
                    }

                    return {
                      data: result
                    }
                  }

                  if (!payload.gasPrice) {
                    return {
                      error:
                        'Gas price is required to update transaction gas fields'
                    }
                  }

                  const result = await ethTxManagerProxy //
                    .setGasPriceAndLimitForUnapprovedTransaction(
                      payload.chainId,
                      payload.txMetaId,
                      payload.gasPrice,
                      payload.gasLimit
                    )

                  if (!result.success) {
                    throw new Error(
                      'Failed to update unapproved transaction: ' +
                        `id=${payload.txMetaId} ` +
                        `gasPrice=${payload.gasPrice}` +
                        `gasLimit=${payload.gasLimit}`
                    )
                  }

                  return {
                    data: result
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    "An error occurred while updating an transaction's gas",
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err
                  ? [TX_CACHE_TAGS.TXS_LIST]
                  : [TX_CACHE_TAGS.ID(arg.txMetaId)]
            }),
            updateUnapprovedTransactionSpendAllowance: mutation<
              { success: boolean },
              UpdateUnapprovedTransactionSpendAllowanceType
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data
                  const { data, success } =
                    await ethTxManagerProxy.makeERC20ApproveData(
                      payload.spenderAddress,
                      payload.allowance
                    )

                  if (!success) {
                    return {
                      error: `Failed making ERC20 approve data, spender: ${
                        payload.spenderAddress //
                      }, allowance: ${
                        payload.allowance //
                      }`
                    }
                  }

                  const result =
                    await ethTxManagerProxy.setDataForUnapprovedTransaction(
                      payload.chainId,
                      payload.txMetaId,
                      data
                    )

                  if (!result.success) {
                    throw new Error(
                      'Failed to set data for unapproved transaction: ' +
                        `chainId=${payload.chainId} txMetaId=${
                          payload.txMetaId
                        } data=${data.join(',')}`
                    )
                  }

                  return {
                    data: result
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Failed to update unapproved transaction:
                id=${payload.txMetaId}
                allowance=${payload.allowance}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.txMetaId)]
            }),
            updateUnapprovedTransactionNonce: mutation<
              { success: boolean },
              UpdateUnapprovedTransactionNonceType
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { ethTxManagerProxy } = baseQuery(undefined).data

                  const result =
                    await ethTxManagerProxy.setNonceForUnapprovedTransaction(
                      payload.chainId,
                      payload.txMetaId,
                      payload.nonce
                    )

                  if (!result.success) {
                    throw new Error('setting nonce failed')
                  }

                  return { data: result }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Failed to update unapproved transaction nonce: ' +
                      `id=${payload.txMetaId} ` +
                      `chainId=${payload.chainId} ` +
                      `nonce=${payload.nonce}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.txMetaId)]
            }),
            retryTransaction: mutation<
              { success: boolean },
              RetryTransactionPayload
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data

                  const result = await txService.retryTransaction(
                    payload.coinType,
                    payload.chainId,
                    payload.transactionId
                  )

                  if (!result.success) {
                    return {
                      error:
                        'Retry transaction failed: ' +
                        `id=${payload.transactionId} ` +
                        `err=${result.errorMessage}`
                    }
                  }

                  return { data: result }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Retry transaction failed: ' +
                      `id=${payload.transactionId} ` +
                      `err=${error}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
            }),
            cancelTransaction: mutation<
              { success: boolean },
              CancelTransactionPayload
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data

                  const result = await txService.speedupOrCancelTransaction(
                    payload.coinType,
                    payload.chainId,
                    payload.transactionId,
                    true
                  )

                  if (!result.success) {
                    throw new Error(result.errorMessage || 'Unknown error')
                  }

                  return {
                    data: { success: result.success }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Cancel transaction failed: ' +
                      `id=${payload.transactionId} ` +
                      `err=${error}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
            }),
            speedupTransaction: mutation<
              { success: boolean },
              SpeedupTransactionPayload
            >({
              queryFn: async (
                payload,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { txService } = baseQuery(undefined).data

                  const result = await txService.speedupOrCancelTransaction(
                    payload.coinType,
                    payload.chainId,
                    payload.transactionId,
                    false
                  )

                  if (!result.success) {
                    return {
                      error:
                        'Speedup transaction failed: ' +
                        `id=${payload.transactionId} ` +
                        `err=${result.errorMessage}`
                    }
                  }

                  return {
                    data: { success: result.success }
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    'Speedup transaction failed: ' +
                      `id=${payload.transactionId} ` +
                      `err=${error}`,
                    error
                  )
                }
              },
              invalidatesTags: (res, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
            }),
            newUnapprovedTxAdded: mutation<
              { success: boolean; txId: string },
              SerializableTransactionInfo
            >({
              queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
                apiProxyFetcher().pageHandler?.showApprovePanelUI()
                return {
                  data: {
                    success: true,
                    txId: arg.id
                  }
                }
              },
              invalidatesTags: (res, err, arg) =>
                // invalidate pending txs
                res
                  ? TX_CACHE_TAGS.LISTS({
                      chainId: arg.chainId,
                      coin: getCoinFromTxDataUnion(arg.txDataUnion),
                      fromAccountId: arg.fromAccountId
                    })
                  : []
            }),
            unapprovedTxUpdated: mutation<
              { success: boolean },
              SerializableTransactionInfo
            >({
              queryFn: async (
                payload,
                { dispatch },
                extraOptions,
                baseQuery
              ) => {
                // no-op (invalidate pending txs)
                return { data: { success: true } }
              },
              invalidatesTags: (_, err, arg) =>
                err ? [] : [TX_CACHE_TAGS.ID(arg.id)]
            }),
            getAddressByteCode: query<
              string,
              { address: string; coin: number; chainId: string }
            >({
              queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
                try {
                  const { jsonRpcService } = baseQuery(undefined).data
                  const { bytecode, error, errorMessage } =
                    await jsonRpcService.getCode(
                      arg.address,
                      arg.coin,
                      arg.chainId
                    )
                  if (error !== 0 && errorMessage) {
                    return {
                      error: errorMessage
                    }
                  }
                  return {
                    data: bytecode
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to fetch bytecode for address: ${arg.address}.`,
                    error
                  )
                }
              }
            }),
            //
            // Transactions Fees
            //
            getGasEstimation1559: query<
              BraveWallet.GasEstimation1559,
              string // chainId
            >({
              queryFn: async (
                chainIdArg,
                { endpoint },
                extraOptions,
                baseQuery
              ) => {
                try {
                  const { data: api } = baseQuery(undefined)
                  const { ethTxManagerProxy } = api

                  const { estimation } =
                    await ethTxManagerProxy.getGasEstimation1559(chainIdArg)

                  if (estimation === null) {
                    const msg = `Failed to fetch gas estimates for chainId: ${
                      chainIdArg //
                    }`
                    console.warn(msg)
                    return {
                      error: msg
                    }
                  }

                  return {
                    data: estimation
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Failed to estimate EVM gas: ${error}`,
                    error
                  )
                }
              },
              providesTags: ['GasEstimation1559']
            }),
            getSolanaEstimatedFee: query<
              string,
              { chainId: string; txId: string }
            >({
              queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
                try {
                  const { solanaTxManagerProxy } = baseQuery(undefined).data
                  const { errorMessage, fee } =
                    await solanaTxManagerProxy.getEstimatedTxFee(
                      arg.chainId,
                      arg.txId
                    )

                  if (!fee) {
                    throw new Error(errorMessage)
                  }

                  return {
                    data: fee.toString()
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to fetch Solana fees - txId: ${arg.txId}`,
                    error
                  )
                }
              },
              providesTags: (res, er, arg) => [
                { type: 'SolanaEstimatedFees', id: arg.txId }
              ]
            }),
            getEthTokenDecimals: query<
              number,
              Pick<BraveWallet.BlockchainToken, 'chainId' | 'contractAddress'>
            >({
              queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
                try {
                  const { jsonRpcService } = baseQuery(undefined).data
                  const { errorMessage, decimals } =
                    await jsonRpcService.getEthTokenDecimals(
                      arg.contractAddress,
                      arg.chainId
                    )

                  if (errorMessage) {
                    throw new Error(errorMessage)
                  }

                  return {
                    data: Number(decimals)
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to fetch token decimals for ${arg.contractAddress}`,
                    error
                  )
                }
              },
              providesTags: (res, err, arg) =>
                err
                  ? ['UNKNOWN_ERROR']
                  : [
                      {
                        type: 'EthTokenDecimals',
                        id: [arg.chainId, arg.contractAddress].join('-')
                      }
                    ]
            }),
            getEthTokenSymbol: query<
              string,
              Pick<BraveWallet.BlockchainToken, 'chainId' | 'contractAddress'>
            >({
              queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
                try {
                  const { jsonRpcService } = baseQuery(undefined).data
                  const { errorMessage, symbol } =
                    await jsonRpcService.getEthTokenSymbol(
                      arg.contractAddress,
                      arg.chainId
                    )

                  if (errorMessage) {
                    throw new Error(errorMessage)
                  }

                  return {
                    data: symbol
                  }
                } catch (error) {
                  return handleEndpointError(
                    endpoint,
                    `Unable to fetch token symbol for ${arg.contractAddress}`,
                    error
                  )
                }
              },
              providesTags: (res, err, arg) =>
                err
                  ? ['UNKNOWN_ERROR']
                  : [
                      {
                        type: 'EthTokenSymbol',
                        id: [arg.chainId, arg.contractAddress].join('-')
                      }
                    ]
            })
          }
        }
      })
      // panel endpoints
      .injectEndpoints({
        endpoints: ({ mutation, query }) => ({
          openPanelUI: mutation<boolean, void>({
            queryFn(arg, api, extraOptions, baseQuery) {
              const { panelHandler } = apiProxyFetcher()
              panelHandler?.showUI()
              return { data: true }
            }
          }),
          closePanelUI: mutation<boolean, void>({
            queryFn(arg, api, extraOptions, baseQuery) {
              const { panelHandler } = apiProxyFetcher()
              panelHandler?.closeUI()
              return { data: true }
            }
          })
        })
      })
      // Wallet management endpoints
      .injectEndpoints({ endpoints: walletEndpoints })
      // Token balance endpoints
      .injectEndpoints({ endpoints: tokenBalancesEndpoints })
      // brave rewards endpoints
      .injectEndpoints({ endpoints: braveRewardsApiEndpoints })
      // tx simulation
      .injectEndpoints({ endpoints: transactionSimulationEndpoints })
      // p3a endpoints
      .injectEndpoints({ endpoints: p3aEndpoints })
      // price history endpoints
      .injectEndpoints({ endpoints: pricingEndpoints })
      // nfts endpoints
      .injectEndpoints({ endpoints: nftsEndpoints })
      // onRamp endpoints
      .injectEndpoints({ endpoints: onRampEndpoints })
      // offRamp endpoints
      .injectEndpoints({ endpoints: offRampEndpoints })
      // coingecko endpoints
      .injectEndpoints({ endpoints: coingeckoEndpoints })
      // token suggestion request endpoints
      .injectEndpoints({ endpoints: tokenSuggestionsEndpoints })
      // QR Code generator endpoints
      .injectEndpoints({ endpoints: qrCodeEndpoints })
      // ENS, SNS, UD Address endpoints
      .injectEndpoints({ endpoints: addressEndpoints })
      // Account management endpoints
      .injectEndpoints({ endpoints: accountEndpoints })
      // Blockchain Network management endpoints
      .injectEndpoints({ endpoints: networkEndpoints })
      // Blockchain Token (User assets) management endpoints
      .injectEndpoints({ endpoints: tokenEndpoints })
      // Coin market endpoints
      .injectEndpoints({ endpoints: coinMarketEndpoints })
      // Fiat currency endpoints
      .injectEndpoints({ endpoints: fiatCurrencyEndpoints })
      // Site permission (connected accounts) endpoints
      .injectEndpoints({ endpoints: sitePermissionEndpoints })
  )
}

export type WalletApi = ReturnType<typeof createWalletApi>
export const walletApi: WalletApi = createWalletApi()

export const {
  middleware: walletApiMiddleware,
  reducer: walletApiReducer,
  reducerPath: walletApiReducerPath,
  // hooks
  useAddAccountMutation,
  useAddUserTokenMutation,
  useApproveERC20AllowanceMutation,
  useApproveHardwareTransactionMutation,
  useApproveOrDeclineTokenSuggestionMutation,
  useApproveTransactionMutation,
  useCancelConnectToSiteMutation,
  useCancelTransactionMutation,
  useCheckExternalWalletPasswordMutation,
  useClosePanelUIMutation,
  useCompleteWalletBackupMutation,
  useConnectToSiteMutation,
  useCreateWalletMutation,
  useEnableEnsOffchainLookupMutation,
  useGenerateReceiveAddressMutation,
  useGetAccountInfosRegistryQuery,
  useGetAccountTokenCurrentBalanceQuery,
  useGetActiveOriginConnectedAccountIdsQuery,
  useGetAddressByteCodeQuery,
  useGetAddressFromNameServiceUrlQuery,
  useGetAllKnownNetworksQuery,
  useGetAutopinEnabledQuery,
  useGetBuyUrlQuery,
  useGetCoingeckoIdQuery,
  useGetCoinMarketQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetERC721MetadataQuery,
  useGetEthAddressChecksumQuery,
  useGetEthTokenDecimalsQuery,
  useGetEthTokenSymbolQuery,
  useGetEVMTransactionSimulationQuery,
  useGetExternalRewardsWalletQuery,
  useGetFVMAddressQuery,
  useGetGasEstimation1559Query,
  useGetHardwareAccountDiscoveryBalanceQuery,
  useGetIpfsGatewayTranslatedNftUrlQuery,
  useGetIPFSUrlFromGatewayLikeUrlQuery,
  useGetIsBase58EncodedSolPubkeyQuery,
  useGetIsTxSimulationOptInStatusQuery,
  useGetIsWalletBackedUpQuery,
  useGetLocalIpfsNodeStatusQuery,
  useGetNetworksRegistryQuery,
  useGetNftDiscoveryEnabledStatusQuery,
  useGetNftMetadataQuery,
  useGetNftPinningStatusQuery,
  useGetNftsPinningStatusQuery,
  useGetOffRampAssetsQuery,
  useGetOnRampAssetsQuery,
  useGetOnRampFiatCurrenciesQuery,
  useGetPendingTokenSuggestionRequestsQuery,
  useGetPriceHistoryQuery,
  useGetPricesHistoryQuery,
  useGetQrCodeImageQuery,
  useGetRewardsBalanceQuery,
  useGetRewardsEnabledQuery,
  useGetSelectedAccountIdQuery,
  useGetSelectedChainQuery,
  useGetSimpleHashSpamNftsQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetSolanaTransactionSimulationQuery,
  useGetSwapSupportedNetworksQuery,
  useGetTokenBalancesForChainIdQuery,
  useGetTokenBalancesRegistryQuery,
  useGetTokenSpotPricesQuery,
  useGetTokensRegistryQuery,
  useGetTransactionsQuery,
  useGetUserTokensRegistryQuery,
  useGetWalletsToImportQuery,
  useHideNetworksMutation,
  useImportFromCryptoWalletsMutation,
  useImportFromMetaMaskMutation,
  useInvalidateAccountInfosMutation,
  useInvalidateSelectedAccountMutation,
  useInvalidateTransactionsCacheMutation,
  useIsEip1559ChangedMutation,
  useLazyGetAccountInfosRegistryQuery,
  useLazyGetAccountTokenCurrentBalanceQuery,
  useLazyGetAddressByteCodeQuery,
  useLazyGetAllKnownNetworksQuery,
  useLazyGetBuyUrlQuery,
  useLazyGetDefaultFiatCurrencyQuery,
  useLazyGetERC721MetadataQuery,
  useLazyGetEVMTransactionSimulationQuery,
  useLazyGetExternalRewardsWalletQuery,
  useLazyGetGasEstimation1559Query,
  useLazyGetIpfsGatewayTranslatedNftUrlQuery,
  useLazyGetIPFSUrlFromGatewayLikeUrlQuery,
  useLazyGetIsTxSimulationOptInStatusQuery,
  useLazyGetNetworksRegistryQuery,
  useLazyGetNftDiscoveryEnabledStatusQuery,
  useLazyGetPendingTokenSuggestionRequestsQuery,
  useLazyGetRewardsBalanceQuery,
  useLazyGetRewardsEnabledQuery,
  useLazyGetSelectedAccountIdQuery,
  useLazyGetSelectedChainQuery,
  useLazyGetSolanaEstimatedFeeQuery,
  useLazyGetSolanaTransactionSimulationQuery,
  useLazyGetSwapSupportedNetworksQuery,
  useLazyGetTokenBalancesForChainIdQuery,
  useLazyGetTokenBalancesRegistryQuery,
  useLazyGetTokenSpotPricesQuery,
  useLazyGetTokensRegistryQuery,
  useLazyGetTransactionsQuery,
  useLazyGetUserTokensRegistryQuery,
  useNewUnapprovedTxAddedMutation,
  useOpenPanelUIMutation,
  usePrefetch,
  useRefreshNetworkInfoMutation,
  useRejectAllTransactionsMutation,
  useRejectTransactionMutation,
  useRemoveSitePermissionMutation,
  useRemoveUserTokenMutation,
  useReportActiveWalletsToP3AMutation,
  useReportOnboardingActionMutation,
  useRequestSitePermissionMutation,
  useRestoreNetworksMutation,
  useRestoreWalletMutation,
  useRetryTransactionMutation,
  useSendBtcTransactionMutation,
  useSendERC20TransferMutation,
  useSendERC721TransferFromMutation,
  useSendETHFilForwarderTransferMutation,
  useSendEthTransactionMutation,
  useSendFilTransactionMutation,
  useSendSolTransactionMutation,
  useSendSPLTransferMutation,
  useSendTransactionMutation,
  useSetAutopinEnabledMutation,
  useSetDefaultFiatCurrencyMutation,
  useSetIsTxSimulationOptInStatusMutation,
  useSetNetworkMutation,
  useSetNftDiscoveryEnabledMutation,
  useSetSelectedAccountMutation,
  useShowRecoveryPhraseMutation,
  useSpeedupTransactionMutation,
  useTransactionStatusChangedMutation,
  useUnapprovedTxUpdatedMutation,
  useUpdateNftSpamStatusMutation,
  useUpdateNftsPinningStatusMutation,
  useUpdateUnapprovedTransactionGasFieldsMutation,
  useUpdateUnapprovedTransactionNonceMutation,
  useUpdateUnapprovedTransactionSpendAllowanceMutation,
  useUpdateUserAssetVisibleMutation,
  useUpdateUserTokenMutation
} = walletApi

// Derived Data Queries

export const useGetMainnetsQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectMainnetNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetNetworksQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectAllNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetOffRampNetworksQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectOffRampNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetOnRampNetworksQuery = (opts?: { skip?: boolean }) => {
  const queryResults = useGetNetworksRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectOnRampNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetVisibleNetworksQuery = (
  arg?: undefined,
  opts?: { skip?: boolean }
) => {
  const queryResults = useGetNetworksRegistryQuery(arg, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      error: res.error,
      data: selectVisibleNetworksFromQueryResult(res)
    }),
    skip: opts?.skip
  })

  return queryResults
}

export const useGetNetworkQuery = (
  args:
    | {
        chainId: string
        coin: BraveWallet.CoinType
      }
    | typeof skipToken
) => {
  return useGetNetworksRegistryQuery(
    args === skipToken ? skipToken : undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading || res.isFetching,
        error: res.error,
        data:
          res.data && args !== skipToken
            ? res.data.entities[networkEntityAdapter.selectId(args)]
            : undefined
      })
    }
  )
}

export type WalletApiSliceState = ReturnType<(typeof walletApi)['reducer']>
export type WalletApiSliceStateFromRoot = { walletApi: WalletApiSliceState }

// panel internals
function navigateToConnectHardwareWallet(
  api: WalletPanelApiProxy,
  store: Pick<Store, 'dispatch' | 'getState'>
) {
  api.panelHandler.setCloseOnDeactivate(false)

  const selectedPanel: string = store.getState()?.panel?.selectedPanel

  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
}
