// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  HardwareWalletResponseCodeType
} from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// actions
import { PanelActions } from '../../../panel/actions'

// utils
import {
  getHasPendingRequests,
  handleEndpointError,
  navigateToConnectHardwareWallet
} from '../../../utils/api-utils'
import { isHardwareAccount } from '../../../utils/account-utils'
import { getLocale } from '../../../../common/locale'
import {
  dialogErrorFromLedgerErrorCode,
  dialogErrorFromTrezorErrorCode,
  signMessageWithHardwareKeyring
} from '../../async/hardware'
import { toByteArrayStringUnion } from '../../../utils/mojo-utils'

interface ProcessSignMessageRequestArgs {
  approved: boolean
  id: number
  signature?: BraveWallet.ByteArrayStringUnion | undefined
  error?: string | undefined
}

export const signingEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getPendingSignMessageRequests: query<
      BraveWallet.SignMessageRequest[],
      void
    >({
      queryFn: async (_arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          const { requests } =
            await api.braveWalletService.getPendingSignMessageRequests()

          return {
            data: requests?.length ? requests : []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending sign-message requests',
            error
          )
        }
      },
      providesTags: ['PendingSignMessageRequests']
    }),

    getPendingSignMessageErrors: query<BraveWallet.SignMessageError[], void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          const { errors } =
            await api.braveWalletService.getPendingSignMessageErrors()

          return {
            data: errors?.length ? errors : []
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending sign-message errors',
            error
          )
        }
      },
      providesTags: ['PendingSignMessageErrors']
    }),

    /** Should work for both hardware & hot wallets */
    processSignMessageRequest: mutation<true, ProcessSignMessageRequestArgs>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          await processSignMessageRequest(api, arg)

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to process pending sign-message request',
            error
          )
        }
      },
      invalidatesTags: ['PendingSignMessageRequests']
    }),

    processSignMessageError: mutation<
      true,
      /** errorId */
      string
    >({
      queryFn: async (errorIdArg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          api.braveWalletService.notifySignMessageErrorProcessed(errorIdArg)

          const { errors } =
            await api.braveWalletService.getPendingSignMessageErrors()

          if (!errors.length) {
            const hasPendingRequests = await getHasPendingRequests()

            if (!hasPendingRequests) {
              api.panelHandler?.closeUI()
            }
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to process pending sign-message request error',
            error
          )
        }
      },
      invalidatesTags: ['PendingSignMessageErrors']
    }),

    signMessageHardware: mutation<
      {
        success: boolean
        hardwareWalletInteractionError?: HardwareWalletResponseCodeType
      },
      {
        request: BraveWallet.SignMessageRequest
        account: BraveWallet.AccountInfo
      }
    >({
      queryFn: async (arg, { endpoint, ...store }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          if (!isHardwareAccount(arg.account.accountId)) {
            api.braveWalletService.notifySignMessageRequestProcessed(
              false,
              arg.request.id,
              null,
              getLocale('braveWalletHardwareAccountNotFound')
            )

            const hasPendingRequests = await getHasPendingRequests()

            if (!hasPendingRequests) {
              api.panelHandler?.closeUI()
            }

            throw new Error('Not a hardware account')
          }

          if (api.panelHandler) {
            navigateToConnectHardwareWallet(api.panelHandler, store)
          }

          const coin = arg.account.accountId.coin
          const info = arg.account.hardware

          if (!info) {
            throw new Error('No hardware account information found')
          }

          const signed = await signMessageWithHardwareKeyring(
            info.vendor,
            info.path,
            arg.request
          )

          if (!signed.success && signed.code) {
            if (signed.code === 'unauthorized') {
              store.dispatch(
                PanelActions.setHardwareWalletInteractionError(signed.code)
              )
              return {
                data: {
                  success: false,
                  hardwareWalletInteractionError: signed.code
                }
              }
            }

            const deviceError =
              info.vendor === BraveWallet.HardwareVendor.kTrezor
                ? dialogErrorFromTrezorErrorCode(signed.code)
                : dialogErrorFromLedgerErrorCode(signed.code)

            if (deviceError !== 'transactionRejected') {
              store.dispatch(
                PanelActions.setHardwareWalletInteractionError(deviceError)
              )
              return {
                data: {
                  success: false,
                  hardwareWalletInteractionError: deviceError
                }
              }
            }
          }

          await processSignMessageRequest(
            api,
            signed.success
              ? {
                  approved: signed.success,
                  id: arg.request.id,
                  signature:
                    coin === BraveWallet.CoinType.SOL
                      ? toByteArrayStringUnion({
                          bytes: [...(signed.payload as Buffer)]
                        })
                      : toByteArrayStringUnion({
                          str: signed.payload as string
                        })
                }
              : {
                  approved: signed.success,
                  id: arg.request.id,
                  error: signed.error as string | undefined
                }
          )

          store.dispatch(PanelActions.navigateToMain())

          const hasPendingRequests = await getHasPendingRequests()

          if (!hasPendingRequests) {
            api.panelHandler?.closeUI()
          }

          return {
            data: {
              success: true
            }
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to sign hardware wallet sign-message request',
            error
          )
        }
      },
      invalidatesTags: ['PendingSignMessageErrors']
    })
  }
}

// internals
async function processSignMessageRequest(
  api: {
    braveWalletService: BraveWallet.BraveWalletServiceRemote
    panelHandler?: BraveWallet.PanelHandlerRemote
  },
  arg: ProcessSignMessageRequestArgs
) {
  api.braveWalletService.notifySignMessageRequestProcessed(
    arg.approved,
    arg.id,
    arg.signature || null,
    arg.error || null
  )

  const hasPendingRequests = await getHasPendingRequests()

  if (!hasPendingRequests) {
    api.panelHandler?.closeUI()
  }
}
