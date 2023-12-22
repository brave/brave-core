// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Store } from '@reduxjs/toolkit'
import { mapLimit } from 'async'

// types
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
} from '../../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionSpendAllowanceType,
  UpdateUnapprovedTransactionNonceType,
  RetryTransactionPayload,
  CancelTransactionPayload,
  SpeedupTransactionPayload
} from '../../constants/action_types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Actions
import { PanelActions } from '../../../panel/actions'

// utils
import { handleEndpointError } from '../../../utils/api-utils'
import {
  getAccountType,
  findAccountByAccountId
} from '../../../utils/account-utils'
import { makeSerializableTransaction } from '../../../utils/model-serialization-utils'
import {
  hasEIP1559Support,
  getCoinFromTxDataUnion
} from '../../../utils/network-utils'
import { TX_CACHE_TAGS } from '../../../utils/query-cache-utils'
import {
  sortTransactionByDate,
  toTxDataUnion,
  shouldReportTransactionP3A
} from '../../../utils/tx-utils'
import {
  signLedgerEthereumTransaction,
  signLedgerFilecoinTransaction,
  signLedgerSolanaTransaction,
  dialogErrorFromLedgerErrorCode,
  signTrezorTransaction
} from '../../async/hardware'

export const transactionEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    // Transactions List
    getTransactions: query<
      SerializableTransactionInfo[],
      {
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
              (tx) => tx.txStatus !== BraveWallet.TransactionStatus.Rejected
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
    invalidateTransactionsCache: mutation<boolean, void>({
      queryFn: () => {
        return { data: true }
      }, // no-op, uses invalidateTags
      invalidatesTags: ['Transactions']
    }),

    // Solana VM
    sendSolTransaction: mutation<
      { success: boolean },
      SendSolTransactionParams
    >({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { solanaTxManagerProxy, txService } = baseQuery(undefined).data

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
            throw new Error(
              'Failed to make SOL system program transfer txData): ' +
                transferTxDataErrorMessage || 'unknown error'
            )
          }

          const { errorMessage, success } =
            await txService.addUnapprovedTransaction(
              toTxDataUnion({ solanaTxData: txData ?? undefined }),
              payload.network.chainId,
              payload.fromAccount.accountId
            )

          if (!success && errorMessage) {
            throw new Error(errorMessage || 'unknown error')
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

    sendSPLTransfer: mutation<{ success: boolean }, SPLTransferFromParams>({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { solanaTxManagerProxy, txService } = baseQuery(undefined).data

          const { errorMessage: transferTxDataErrorMessage, txData } =
            await solanaTxManagerProxy.makeTokenProgramTransferTxData(
              payload.network.chainId,
              payload.splTokenMintAddress,
              payload.fromAccount.address,
              payload.to,
              BigInt(payload.value)
            )

          if (!txData) {
            throw new Error(
              `Failed making SPL transfer data: ${transferTxDataErrorMessage}`
            )
          }

          const { errorMessage, success } =
            await txService.addUnapprovedTransaction(
              toTxDataUnion({ solanaTxData: txData }),
              payload.network.chainId,
              payload.fromAccount.accountId
            )

          if (!success) {
            throw new Error(errorMessage || 'unknown error')
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

    sendSolanaSerializedTransaction: mutation<
      boolean, // success,
      {
        encodedTransaction: string
        chainId: string
        accountId: BraveWallet.AccountId
        txType: BraveWallet.TransactionType
        sendOptions?: BraveWallet.SolanaSendTransactionOptions
      }
    >({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { solanaTxManagerProxy, txService } = api
          const result =
            await solanaTxManagerProxy.makeTxDataFromBase64EncodedTransaction(
              payload.encodedTransaction,
              payload.txType,
              payload.sendOptions || null
            )

          if (result.error !== BraveWallet.ProviderError.kSuccess) {
            throw new Error(
              `Failed to sign Solana message: ${result.errorMessage}`
            )
          }

          const { errorMessage, success } =
            await txService.addUnapprovedTransaction(
              toTxDataUnion({ solanaTxData: result.txData ?? undefined }),
              payload.chainId,
              payload.accountId
            )

          if (!success) {
            throw new Error(
              `Error creating Solana transaction: ${errorMessage}`
            )
          }

          return {
            data: success
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to send Solana serialized transaction',
            error
          )
        }
      }
    }),

    // BTC
    sendBtcTransaction: mutation<
      { success: boolean },
      SendBtcTransactionParams
    >({
      queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
        try {
          const { txService } = baseQuery(undefined).data

          const btcTxData: BraveWallet.BtcTxData = {
            to: payload.to,
            amount: BigInt(payload.value),
            sendingMaxAmount: payload.sendingMaxValue,
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
            throw new Error(errorMessage || 'unknown error')
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

    // ZEC
    sendZecTransaction: mutation<
      { success: boolean },
      SendZecTransactionParams
    >({
      queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
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
            throw new Error(errorMessage || 'unknown error')
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

    // FIL
    sendFilTransaction: mutation<
      { success: boolean },
      SendFilTransactionParams
    >({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
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

          if (!success) {
            throw new Error(errorMessage || 'unknown error')
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

    // FEVM
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
          const { ethTxManagerProxy, txService } = baseQuery(undefined).data
          const { data, success } =
            await ethTxManagerProxy.makeFilForwarderTransferData(payload.to)

          if (!success) {
            throw new Error(
              'Failed making FilForwarder transferFrom data, from: ' +
                payload.fromAccount.address +
                ' to: ' +
                payload.to
            )
          }

          const result = await sendEvmTransaction({
            payload: {
              network: payload.network,
              fromAccount: payload.fromAccount,
              to: payload.contractAddress,
              value: payload.value,
              gas: payload.gas,
              gasPrice: payload.gasPrice,
              maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
              maxFeePerGas: payload.maxFeePerGas,
              data
            },
            txService
          })

          return result
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

    // EVM Transactions
    sendEvmTransaction: mutation<
      { success: boolean },
      SendEthTransactionParams
    >({
      queryFn: async (
        payload,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const { txService } = baseQuery(undefined).data

          const result = await sendEvmTransaction({
            payload,
            txService
          })

          return result
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to send EVM Transaction',
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

    sendERC20Transfer: mutation<{ success: boolean }, ER20TransferParams>({
      queryFn: async (
        payload,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const { ethTxManagerProxy, txService } = baseQuery(undefined).data
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

          const result = await sendEvmTransaction({
            payload: {
              network: payload.network,
              fromAccount: payload.fromAccount,
              to: payload.contractAddress,
              value: '0x0',
              gas: payload.gas,
              gasPrice: payload.gasPrice,
              maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
              maxFeePerGas: payload.maxFeePerGas,
              data
            },
            txService
          })

          return result
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to send ERC20 Transfer',
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
          const { ethTxManagerProxy, txService } = baseQuery(undefined).data
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

          const result = await sendEvmTransaction({
            payload: {
              network: payload.network,
              fromAccount: payload.fromAccount,
              to: payload.contractAddress,
              value: '0x0',
              gas: payload.gas,
              gasPrice: payload.gasPrice,
              maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
              maxFeePerGas: payload.maxFeePerGas,
              data
            },
            txService
          })

          return result
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
          chainId: arg.network.chainId,
          coin: arg.fromAccount.accountId.coin,
          fromAccountId: arg.fromAccount.accountId
        })
    }),

    approveERC20Allowance: mutation<{ success: boolean }, ApproveERC20Params>({
      queryFn: async (
        payload,
        { dispatch, endpoint },
        extraOptions,
        baseQuery
      ) => {
        try {
          const { ethTxManagerProxy, txService } = baseQuery(undefined).data

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

          const result = await sendEvmTransaction({
            payload: {
              network: payload.network,
              fromAccount: payload.fromAccount,
              to: payload.contractAddress,
              value: '0x0',
              data
            },
            txService
          })

          return result
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to approve ERC20 allowance',
            error
          )
        }
      },
      invalidatesTags: (res, err, arg) =>
        TX_CACHE_TAGS.LISTS({
          chainId: arg.network.chainId,
          coin: BraveWallet.CoinType.ETH,
          fromAccountId: arg.fromAccount.accountId
        })
    }),

    // Transactions Management
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
              ...(arg.txStatus === BraveWallet.TransactionStatus.Confirmed
                ? ([
                    'UserBlockchainTokens', // refresh all user tokens
                    'AccountTokenCurrentBalance',
                    'TokenSpotPrices'
                  ] as const)
                : [])
            ]
    }),

    approveTransaction: mutation<
      { success: boolean },
      Pick<SerializableTransactionInfo, 'id' | 'chainId' | 'txType'> & {
        coinType: BraveWallet.CoinType
      }
    >({
      queryFn: async (txInfo, { endpoint }, extraOptions, baseQuery) => {
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
            throw new Error(`${error}: ${result.errorMessage}`)
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
            throw new Error(
              'failed to approve hardware transaction - ' +
                'account not found or is not hardware: ' +
                txInfo.fromAccountId.uniqueKey
            )
          }

          const hardwareAccount: BraveWallet.HardwareInfo =
            foundAccount.hardware

          if (apiProxy.panelHandler) {
            navigateToConnectHardwareWallet(apiProxy.panelHandler, store)
          }

          if (hardwareAccount.vendor === BraveWallet.LEDGER_HARDWARE_VENDOR) {
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
                ;({ success, error, code } = await signLedgerSolanaTransaction(
                  apiProxy,
                  hardwareAccount.path,
                  txInfo,
                  foundAccount.accountId.coin
                ))
                break
              default:
                await store.dispatch(PanelActions.navigateToMain())
                throw new Error(`unsupported coin type for hardware approval`)
            }
            if (success) {
              store.dispatch(PanelActions.setSelectedTransactionId(txInfo.id))
              store.dispatch(PanelActions.navigateTo('transactionDetails'))
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
                await apiProxy.txService.rejectTransaction(
                  getCoinFromTxDataUnion(txInfo.txDataUnion),
                  txInfo.chainId,
                  txInfo.id
                )
                store.dispatch(PanelActions.navigateToMain())
                return {
                  data: { success: true }
                }
              }

              store.dispatch(
                PanelActions.setHardwareWalletInteractionError(deviceError)
              )
              throw new Error('device error: ' + deviceError)
            }

            if (error) {
              console.log(error)
              store.dispatch(PanelActions.navigateToMain())

              throw new Error(
                typeof error === 'object'
                  ? JSON.stringify(error)
                  : error || 'unknown error'
              )
            }
          } else if (
            hardwareAccount.vendor === BraveWallet.TREZOR_HARDWARE_VENDOR
          ) {
            const { success, error, deviceError } = await signTrezorTransaction(
              apiProxy,
              hardwareAccount.path,
              txInfo
            )
            if (success) {
              store.dispatch(PanelActions.setSelectedTransactionId(txInfo.id))
              store.dispatch(PanelActions.navigateTo('transactionDetails'))
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
            await apiProxy.txService.rejectTransaction(
              getCoinFromTxDataUnion(txInfo.txDataUnion),
              txInfo.chainId,
              txInfo.id
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

    rejectTransactions: mutation<
      { success: boolean },
      Array<
        Pick<BraveWallet.TransactionInfo, 'id' | 'chainId'> & {
          coinType: BraveWallet.CoinType
        }
      >
    >({
      queryFn: async (txs, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { txService } = baseQuery(undefined).data

          await mapLimit(
            txs,
            10,
            async (
              tx: Pick<BraveWallet.TransactionInfo, 'id' | 'chainId'> & {
                coinType: BraveWallet.CoinType
              }
            ) => {
              await txService.rejectTransaction(tx.coinType, tx.chainId, tx.id)
              return true
            }
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
        err ? [] : arg.map(({ id }) => TX_CACHE_TAGS.ID(id))
    }),

    updateUnapprovedTransactionGasFields: mutation<
      { success: boolean },
      UpdateUnapprovedTransactionGasFieldsType
    >({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
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
              error: 'Gas price is required to update transaction gas fields'
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
        err ? [TX_CACHE_TAGS.TXS_LIST] : [TX_CACHE_TAGS.ID(arg.txMetaId)]
    }),

    updateUnapprovedTransactionSpendAllowance: mutation<
      { success: boolean },
      UpdateUnapprovedTransactionSpendAllowanceType
    >({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { ethTxManagerProxy } = baseQuery(undefined).data
          const { data, success } =
            await ethTxManagerProxy.makeERC20ApproveData(
              payload.spenderAddress,
              payload.allowance
            )

          if (!success) {
            throw new Error(
              `Failed making ERC20 approve data, spender: ${
                payload.spenderAddress //
              }, allowance: ${
                payload.allowance //
              }`
            )
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
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
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

    retryTransaction: mutation<{ success: boolean }, RetryTransactionPayload>({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { txService } = baseQuery(undefined).data

          const result = await txService.retryTransaction(
            payload.coinType,
            payload.chainId,
            payload.transactionId
          )

          if (!result.success) {
            throw new Error(result.errorMessage || 'unknown error')
          }

          return { data: result }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Retry transaction failed: ' + `id=${payload.transactionId} `,
            error
          )
        }
      },
      invalidatesTags: (res, err, arg) =>
        err ? [] : [TX_CACHE_TAGS.ID(arg.transactionId)]
    }),

    cancelTransaction: mutation<{ success: boolean }, CancelTransactionPayload>(
      {
        queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
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
      }
    ),

    speedupTransaction: mutation<
      { success: boolean },
      SpeedupTransactionPayload
    >({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { txService } = baseQuery(undefined).data

          const result = await txService.speedupOrCancelTransaction(
            payload.coinType,
            payload.chainId,
            payload.transactionId,
            false
          )

          if (!result.success) {
            throw new Error(result.errorMessage || 'unknown error')
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
        const { pageHandler } = baseQuery(undefined).data
        pageHandler?.showApprovePanelUI()
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
      queryFn: async (payload, { dispatch }, extraOptions, baseQuery) => {
        // no-op (invalidate pending txs)
        return { data: { success: true } }
      },
      invalidatesTags: (_, err, arg) => (err ? [] : [TX_CACHE_TAGS.ID(arg.id)])
    }),

    // Transactions Fees
    getGasEstimation1559: query<
      BraveWallet.GasEstimation1559,
      string // chainId
    >({
      queryFn: async (chainIdArg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { ethTxManagerProxy } = api

          const { estimation } = await ethTxManagerProxy.getGasEstimation1559(
            chainIdArg
          )

          if (estimation === null) {
            throw new Error(
              `Failed to fetch gas estimates for chainId: ${
                chainIdArg //
              }`
            )
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

    getSolanaEstimatedFee: query<string, { chainId: string; txId: string }>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { solanaTxManagerProxy } = baseQuery(undefined).data
          const { errorMessage, fee } =
            await solanaTxManagerProxy.getEstimatedTxFee(arg.chainId, arg.txId)

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
    })
  }
}

// internals
async function sendEvmTransaction({
  payload,
  txService
}: {
  payload: SendEthTransactionParams
  txService: BraveWallet.TxServiceRemote
}): Promise<{ data: { success: boolean } }> {
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
    hasEIP1559Support(getAccountType(payload.fromAccount), payload.network)

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

  const { errorMessage, success } = await txService.addUnapprovedTransaction(
    isEIP1559
      ? toTxDataUnion({ ethTxData1559: txData1559 })
      : toTxDataUnion({ ethTxData: txData }),
    payload.network.chainId,
    payload.fromAccount.accountId
  )

  if (!success && errorMessage) {
    throw new Error(
      `Failed to create Evm transaction: ${
        errorMessage || 'unknown error'
      } ::payload:: ${JSON.stringify(payload)}`
    )
  }

  return {
    data: { success }
  }
}

// panel internals
function navigateToConnectHardwareWallet(
  panelHandler: BraveWallet.PanelHandlerRemote,
  store: Pick<Store, 'dispatch' | 'getState'>
) {
  panelHandler.setCloseOnDeactivate(false)

  const selectedPanel: string = store.getState()?.panel?.selectedPanel

  if (selectedPanel === 'connectHardwareWallet') {
    return
  }

  store.dispatch(PanelActions.navigateTo('connectHardwareWallet'))
  store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
}
