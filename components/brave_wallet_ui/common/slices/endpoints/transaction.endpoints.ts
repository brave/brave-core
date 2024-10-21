// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
  SPLTransferFromParams,
  TransactionInfoLookup
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
import {
  getHasPendingRequests,
  handleEndpointError,
  navigateToConnectHardwareWallet
} from '../../../utils/api-utils'
import {
  findAccountByAccountId,
  isHardwareAccount
} from '../../../utils/account-utils'
import { makeSerializableTransaction } from '../../../utils/model-serialization-utils'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { TX_CACHE_TAGS } from '../../../utils/query-cache-utils'
import { sortTransactionByDate, toTxDataUnion } from '../../../utils/tx-utils'
import {
  signLedgerEthereumTransaction,
  signLedgerFilecoinTransaction,
  signLedgerSolanaTransaction,
  dialogErrorFromLedgerErrorCode,
  signTrezorTransaction,
  signSolTransactionWithHardwareKeyring,
  signLedgerBitcoinTransaction
} from '../../async/hardware'
import { getLocale } from '../../../../common/locale'

interface ProcessSignSolTransactionsRequestPayload {
  approved: boolean
  id: number
  hwSignatures: BraveWallet.SolanaSignature[]
  error: string | null
}

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

    getTransaction: query<
      SerializableTransactionInfo | null,
      TransactionInfoLookup
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { transactionInfo: tx } =
            await api.txService.getTransactionInfo(arg.coin, arg.id)
          return {
            data: tx ? makeSerializableTransaction(tx) : null
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Unable to get transaction (${arg.id})`,
            error
          )
        }
      },
      providesTags: (res, err, arg) => (err ? [] : [TX_CACHE_TAGS.ID(arg.id)])
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

    /** works for SPL tokens and Solana compressed NFTs */
    sendSPLTransfer: mutation<{ success: boolean }, SPLTransferFromParams>({
      queryFn: async (payload, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { solanaTxManagerProxy, txService } = baseQuery(undefined).data

          const { errorMessage: transferTxDataErrorMessage, txData } =
            payload.isCompressedNft
              ? await solanaTxManagerProxy.makeBubbleGumProgramTransferTxData(
                  payload.network.chainId,
                  payload.splTokenMintAddress,
                  payload.fromAccount.address,
                  payload.to
                )
              : await solanaTxManagerProxy.makeTokenProgramTransferTxData(
                  payload.network.chainId,
                  payload.splTokenMintAddress,
                  payload.fromAccount.address,
                  payload.to,
                  BigInt(payload.value),
                  payload.decimals
                )

          if (!txData) {
            throw new Error(
              `Failed making ${
                payload.isCompressedNft ? 'Compressed NFT' : 'SPL'
              } transfer data: ${transferTxDataErrorMessage}`
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
            `${
              payload.isCompressedNft ? 'Compressed NFT' : 'SPL'
            } Transfer failed:
                  to: ${payload.to}
                  value: ${payload.value}`,
            error
          )
        }
      },
      invalidatesTags: (res, err, arg) => [
        ...TX_CACHE_TAGS.LISTS({
          chainId: null,
          coin: arg.fromAccount.accountId.coin,
          fromAccountId: arg.fromAccount.accountId
        }),
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
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

    getPendingSignSolTransactionsRequests: query<
      BraveWallet.SignSolTransactionsRequest[],
      void
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          const { requests } =
            await api.braveWalletService.getPendingSignSolTransactionsRequests()

          return {
            data: requests
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending Sign-All-Transactions Requests',
            error
          )
        }
      },
      providesTags: ['PendingSignSolTransactionsRequests']
    }),

    processSignSolTransactionsRequest: mutation<
      /** success */
      true,
      ProcessSignSolTransactionsRequestPayload
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          await processSignSolTransactionsRequest(
            api.braveWalletService,
            arg,
            api.panelHandler
          )

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to Sign-All-Transactions',
            error
          )
        }
      },
      invalidatesTags: ['PendingSignSolTransactionsRequests']
    }),

    processSignSolTransactionsRequestHardware: mutation<
      { success: boolean; errorCode?: string | number },
      {
        request: BraveWallet.SignSolTransactionsRequest
      }
    >({
      queryFn: async (arg, { endpoint, ...store }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { braveWalletService, panelHandler } = api

          if (!isHardwareAccount(arg.request.fromAccountId)) {
            const errorString = getLocale('braveWalletHardwareAccountNotFound')

            braveWalletService.notifySignSolTransactionsRequestProcessed(
              false,
              arg.request.id,
              [],
              errorString
            )

            const hasPendingRequests = await getHasPendingRequests()

            if (!hasPendingRequests) {
              api.panelHandler?.closeUI()
            }

            return {
              data: {
                success: false,
                errorCode: errorString
              }
            }
          }

          const accountsRegistry = await cache.getAccountsRegistry()
          const info = findAccountByAccountId(
            arg.request.fromAccountId,
            accountsRegistry
          )?.hardware

          if (!info) {
            throw new Error('No hardware account info')
          }

          if (panelHandler) {
            navigateToConnectHardwareWallet(panelHandler, store)
          }

          // Send serialized requests to hardware keyring to sign.
          const payload: ProcessSignSolTransactionsRequestPayload = {
            approved: true,
            id: arg.request.id,
            hwSignatures: [],
            error: null
          }

          for (const rawMessage of arg.request.rawMessages) {
            const signed = await signSolTransactionWithHardwareKeyring(
              info.vendor,
              info.path,
              Buffer.from(rawMessage),
              () => {
                // dismiss hardware connect screen
                store.dispatch(PanelActions.navigateToMain())
              }
            )

            if (!signed.success) {
              if (signed.code && signed.code === 'unauthorized') {
                store.dispatch(
                  PanelActions.setHardwareWalletInteractionError(signed.code)
                )
                return {
                  data: {
                    success: false,
                    errorCode: signed.code
                  }
                }
              }
              payload.approved = false
              payload.hwSignatures = []
              payload.error = signed.error
              break
            } else {
              payload.hwSignatures?.push(signed.signature)
            }
          }

          await processSignSolTransactionsRequest(
            braveWalletService,
            payload,
            panelHandler
          )

          return {
            data: {
              success: true
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to Sign-All-Transactions (Hardware)',
            error
          )
        }
      },
      invalidatesTags: ['PendingSignSolTransactionsRequests']
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
            useShieldedPool: payload.useShieldedPool,
            to: payload.to,
            memo: undefined,
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
              gasLimit: payload.gasLimit,
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
              gasLimit: payload.gasLimit,
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
              gasLimit: payload.gasLimit,
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
              gasLimit: '',
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
                    'TokenSpotPrices',
                    'TokenBalances',
                    'TokenBalancesForChainId',
                    'AccountTokenCurrentBalance'
                  ] as const)
                : [])
            ]
    }),

    approveTransaction: mutation<
      {
        success: boolean
        errorUnion: BraveWallet.ProviderErrorUnion
        errorMessage: string
      },
      Pick<SerializableTransactionInfo, 'id' | 'chainId' | 'txType'> & {
        coinType: BraveWallet.CoinType
      }
    >({
      queryFn: async (txInfo, { endpoint }, extraOptions, baseQuery) => {
        try {
          const api = baseQuery(undefined).data
          const { txService } = api
          const result: {
            status: boolean
            errorUnion: BraveWallet.ProviderErrorUnion
            errorMessage: string
          } = await txService.approveTransaction(
            txInfo.coinType,
            txInfo.chainId,
            txInfo.id
          )

          return {
            data: {
              success: result.status,
              errorMessage: result.errorMessage,
              errorUnion: result.errorUnion
            }
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

          if (hardwareAccount.vendor === BraveWallet.HardwareVendor.kLedger) {
            let result
            switch (foundAccount.accountId.coin) {
              case BraveWallet.CoinType.ETH:
                result = await signLedgerEthereumTransaction(
                  apiProxy,
                  hardwareAccount.path,
                  txInfo.id
                )
                break
              case BraveWallet.CoinType.FIL:
                result = await signLedgerFilecoinTransaction(
                  apiProxy,
                  txInfo.id
                )
                break
              case BraveWallet.CoinType.BTC:
                result = await signLedgerBitcoinTransaction(apiProxy, txInfo.id)
                break
              case BraveWallet.CoinType.SOL:
                result = await signLedgerSolanaTransaction(
                  apiProxy,
                  hardwareAccount.path,
                  txInfo.id
                )
                break
              default:
                await store.dispatch(PanelActions.navigateToMain())
                throw new Error(`unsupported coin type for hardware approval`)
            }
            if (result.success) {
              store.dispatch(
                PanelActions.setSelectedTransactionId({
                  chainId: txInfo.chainId,
                  coin: getCoinFromTxDataUnion(txInfo.txDataUnion),
                  id: txInfo.id
                })
              )
              store.dispatch(PanelActions.navigateTo('transactionStatus'))
              apiProxy.panelHandler?.setCloseOnDeactivate(true)
              return {
                data: { success: true }
              }
            }
            const { error, code } = result

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
            hardwareAccount.vendor === BraveWallet.HardwareVendor.kTrezor
          ) {
            const result = await signTrezorTransaction(
              apiProxy,
              hardwareAccount.path,
              txInfo
            )
            if (result.success) {
              store.dispatch(
                PanelActions.setSelectedTransactionId({
                  chainId: txInfo.chainId,
                  coin: getCoinFromTxDataUnion(txInfo.txDataUnion),
                  id: txInfo.id
                })
              )
              store.dispatch(PanelActions.navigateTo('transactionStatus'))
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

            if (result.code === 'deviceBusy') {
              // do nothing as the operation is already in progress
              return {
                data: { success: true }
              }
            }

            console.log(result.error)
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
        err
          ? [TX_CACHE_TAGS.TXS_LIST]
          : [TX_CACHE_TAGS.ID(arg.txMetaId), 'GasEstimation1559']
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
            await solanaTxManagerProxy.getSolanaTxFeeEstimation(
              arg.chainId,
              arg.txId
            )

          if (!fee) {
            throw new Error(errorMessage)
          }

          const priorityFee =
            (BigInt(fee.computeUnits) * BigInt(fee.feePerComputeUnit)) /
            BigInt(BraveWallet.MICRO_LAMPORTS_PER_LAMPORT)
          const totalFee = BigInt(fee.baseFee) + priorityFee

          return {
            data: totalFee.toString()
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
  const params: BraveWallet.NewEvmTransactionParams = {
    chainId: payload.network.chainId,
    from: payload.fromAccount.accountId,
    gasLimit: payload.gasLimit,
    to: payload.to,
    value: payload.value,
    data: payload.data
  }

  const { errorMessage, success } = await txService.addUnapprovedEvmTransaction(
    params
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

async function processSignSolTransactionsRequest(
  braveWalletService: BraveWallet.BraveWalletServiceRemote,
  arg: ProcessSignSolTransactionsRequestPayload,
  panelHandler: BraveWallet.PanelHandlerRemote | undefined
) {
  braveWalletService.notifySignSolTransactionsRequestProcessed(
    arg.approved,
    arg.id,
    arg.hwSignatures,
    arg.error || null
  )

  const hasPendingRequests = await getHasPendingRequests()

  if (!hasPendingRequests) {
    panelHandler?.closeUI()
  }
}
