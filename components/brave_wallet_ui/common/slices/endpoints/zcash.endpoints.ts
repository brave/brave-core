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

type MakeAccountShieldedPayloadType = {
  accountId: BraveWallet.AccountId
  accountBirthdayBlock: number
}

type GetZCashBalancePayloadType = {
  chainId: string
  accountId: BraveWallet.AccountId
}

export const zcashEndpoints = ({
  query,
  mutation,
}: WalletApiEndpointBuilderParams) => {
  return {
    makeAccountShielded: mutation<true, MakeAccountShieldedPayloadType>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        const { accountId, accountBirthdayBlock } = args
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { errorMessage } = await zcashWalletService.makeAccountShielded(
            accountId,
            accountBirthdayBlock,
          )

          if (errorMessage) {
            return handleEndpointError(
              endpoint,
              'Error making account shielded: ',
              errorMessage,
            )
          }

          return {
            data: true,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error making account shielded: ',
            error,
          )
        }
      },
      invalidatesTags: [
        'ZCashAccountInfo',
        'IsShieldingAvailable',
        'ZcashChainTipStatus',
      ],
    }),
    getZCashAccountInfo: query<
      BraveWallet.ZCashAccountInfo | null,
      BraveWallet.AccountId
    >({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { accountInfo } =
            await zcashWalletService.getZCashAccountInfo(args)

          return {
            data: accountInfo,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting ZCash account info: ',
            error,
          )
        }
      },
      providesTags: ['ZCashAccountInfo'],
    }),
    getZCashBalance: query<
      BraveWallet.ZCashBalance | null,
      GetZCashBalancePayloadType
    >({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { balance } = await zcashWalletService.getBalance(
            args.accountId,
          )

          return {
            data: balance,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting ZCash balance info: ',
            error,
          )
        }
      },
      providesTags: ['ZCashBalance'],
    }),
    getIsShieldingAvailable: query<boolean, BraveWallet.AccountId[]>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const accountInfos = await mapLimit(
            args,
            10,
            async (accountId: BraveWallet.AccountId) =>
              await zcashWalletService.getZCashAccountInfo(accountId),
          )

          return {
            data: !accountInfos.some(
              (info) => info.accountInfo?.accountShieldBirthday,
            ),
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting is shielding available: ',
            error,
          )
        }
      },
      providesTags: ['IsShieldingAvailable'],
    }),
    getAvailableShieldedAccount: query<
      BraveWallet.ZCashAccountInfo | null,
      BraveWallet.AccountId[]
    >({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const accountInfos = await mapLimit(
            args,
            10,
            async (accountId: BraveWallet.AccountId) =>
              await zcashWalletService.getZCashAccountInfo(accountId),
          )

          return {
            data:
              accountInfos.filter(
                (info) => info.accountInfo?.accountShieldBirthday,
              )[0]?.accountInfo ?? null,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting available shielded account: ',
            error,
          )
        }
      },
      providesTags: ['AvailableShieldedAccount'],
    }),
    getChainTipStatus: query<
      BraveWallet.ZCashChainTipStatus | null,
      BraveWallet.AccountId
    >({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { status } = await zcashWalletService.getChainTipStatus(args)

          return {
            data: status,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting Zcash Chain Tip Status info: ',
            error,
          )
        }
      },
      providesTags: ['ZcashChainTipStatus'],
    }),

    startShieldSync: mutation<true, BraveWallet.AccountId>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data

          const { errorMessage } = await zcashWalletService.startShieldSync(
            args,
            0,
          )

          if (errorMessage) {
            return handleEndpointError(
              endpoint,
              'Error starting shield sync: ',
              errorMessage,
            )
          }

          return {
            data: true,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error starting shield sync: ',
            error,
          )
        }
      },
      invalidatesTags: ['IsSyncInProgress'],
    }),
    clearChainTipStatusCache: mutation<true, void>({
      queryFn: async (_args, { endpoint }, _extraOptions, baseQuery) => {
        return {
          data: true,
        }
      },
      invalidatesTags: ['ZcashChainTipStatus'],
    }),
    clearZCashBalanceCache: mutation<true, void>({
      queryFn: async (_args, { endpoint }, _extraOptions, baseQuery) => {
        return {
          data: true,
        }
      },
      invalidatesTags: ['ZCashBalance'],
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
              errorMessage,
            )
          }

          return {
            data: true,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error stopping shield sync: ',
            error,
          )
        }
      },
      invalidatesTags: [
        'ZcashChainTipStatus',
        'IsSyncInProgress',
        'TokenBalances',
      ],
    }),

    getIsSyncInProgress: query<boolean, BraveWallet.AccountId>({
      queryFn: async (args, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { zcashWalletService } = baseQuery(undefined).data
          const { result } = await zcashWalletService.isSyncInProgress(args)
          return {
            data: result,
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Error getting is sync in progress: ',
            error,
          )
        }
      },
      providesTags: ['IsSyncInProgress'],
    }),
  }
}
