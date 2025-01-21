// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { handleEndpointError } from '../../../utils/api-utils'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'
import { mapLimit } from 'async'

export const zcashEndpoints = ({
  query,
  mutation
}: WalletApiEndpointBuilderParams) => {
  return {
    makeAccountShielded: mutation<true, BraveWallet.AccountId>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { errorMessage } = await zcashWalletService.makeAccountShielded(
            args,
            0
          )

          if (errorMessage) {
            return handleEndpointError(
              endpoint,
              'Error making account shielded: ',
              errorMessage
            )
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error making account shielded: ',
            error
          )
        }
      },
      invalidatesTags: ['ZCashAccountInfo', 'IsShieldingAvailable']
    }),
    getZCashAccountInfo: query<
      BraveWallet.ZCashAccountInfo | null,
      BraveWallet.AccountId
    >({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { accountInfo } = await zcashWalletService.getZCashAccountInfo(
            args
          )

          return {
            data: accountInfo
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting ZCash account info: ',
            error
          )
        }
      },
      providesTags: ['ZCashAccountInfo']
    }),
    getIsShieldingAvailable: query<boolean, BraveWallet.AccountId[]>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const accountInfos = await mapLimit(
            args,
            10,
            async (accountId: BraveWallet.AccountId) =>
              await zcashWalletService.getZCashAccountInfo(accountId)
          )

          return {
            data: !accountInfos.some(
              (info) => info.accountInfo?.accountShieldBirthday
            )
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting is shielding available: ',
            error
          )
        }
      },
      providesTags: ['IsShieldingAvailable']
    }),

    getChainTipStatus: query<
      BraveWallet.ZCashChainTipStatus | null,
      BraveWallet.AccountId
    >({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { status } = await zcashWalletService.getChainTipStatus(
            args,
            BraveWallet.Z_CASH_MAINNET
          )

          return {
            data: status
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting Zcash Chain Tip Status info: ',
            error
          )
        }
      },
      providesTags: ['ZcashChainTipStatus']
    }),

    startShieldSync: mutation<true, BraveWallet.AccountId>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { errorMessage } = await zcashWalletService.startShieldSync(
            args,
            0
          )

          if (errorMessage) {
            return handleEndpointError(
              endpoint,
              'Error starting shield sync: ',
              errorMessage
            )
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error starting shield sync: ',
            error
          )
        }
      },
      invalidatesTags: []
    }),

    stopShieldSync: mutation<true, BraveWallet.AccountId>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { errorMessage } = await zcashWalletService.stopShieldSync(args)

          if (errorMessage) {
            return handleEndpointError(
              endpoint,
              'Error stopping shield sync: ',
              errorMessage
            )
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error stopping shield sync: ',
            error
          )
        }
      },
      invalidatesTags: ['ZcashChainTipStatus']
    })
  }
}
