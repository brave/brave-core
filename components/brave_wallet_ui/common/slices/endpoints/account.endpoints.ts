// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// constants
import {
  BitcoinNetwork,
  BraveWallet,
  FilecoinNetwork
} from '../../../constants/types'
import { NETWORK_TAG_IDS } from './network.endpoints'

// types
import { WalletApiEndpointBuilderParams } from '../api-base.slice'
import { AccountInfoEntityState } from '../entities/account-info.entity'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export const ACCOUNT_TAG_IDS = {
  REGISTRY: 'REGISTRY',
  SELECTED: 'SELECTED'
} as const

export const accountEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getAccountInfosRegistry: query<AccountInfoEntityState, void>({
      queryFn: async (_arg, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { cache } = baseQuery(undefined)
          return {
            data: await cache.getAccountsRegistry()
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to fetch accounts',
            error
          )
        }
      },
      providesTags: (res, err) =>
        err
          ? ['UNKNOWN_ERROR']
          : [{ type: 'AccountInfos', id: ACCOUNT_TAG_IDS.REGISTRY }]
    }),

    invalidateAccountInfos: mutation<boolean, void>({
      queryFn: (arg, api, extraOptions, baseQuery) => {
        baseQuery(undefined).cache.clearAccountsRegistry()
        return { data: true }
      },
      invalidatesTags: [
        { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
        { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.REGISTRY },
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    invalidateSelectedAccount: mutation<boolean, void>({
      queryFn: (arg, api, extraOptions, baseQuery) => {
        baseQuery(undefined).cache.clearSelectedAccount()
        return { data: true }
      },
      invalidatesTags: [
        { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
        { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }
      ]
    }),

    setSelectedAccount: mutation<BraveWallet.AccountId, BraveWallet.AccountId>({
      queryFn: async (accountId, { endpoint }, extraOptions, baseQuery) => {
        try {
          const {
            cache,
            data: { keyringService }
          } = baseQuery(undefined)

          await keyringService.setSelectedAccount(accountId)
          cache.clearSelectedAccount()

          return {
            data: accountId
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to select account (${accountId})`,
            error
          )
        }
      },
      invalidatesTags: [
        { type: 'Network', id: NETWORK_TAG_IDS.SELECTED },
        { type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }
      ]
    }),

    getSelectedAccountId: query<BraveWallet.AccountId | null, void>({
      queryFn: async (arg, { dispatch }, extraOptions, baseQuery) => {
        return {
          data:
            (await baseQuery(undefined).cache.getAllAccounts()).selectedAccount
              ?.accountId || null
        }
      },
      providesTags: [{ type: 'AccountInfos', id: ACCOUNT_TAG_IDS.SELECTED }]
    }),

    addAccount: mutation<
      BraveWallet.AccountInfo,
      {
        accountName: string
        keyringId: BraveWallet.KeyringId
        coin: BraveWallet.CoinType
      }
    >({
      queryFn: async (args, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { accountInfo } = await api.keyringService.addAccount(
            args.coin,
            args.keyringId,
            args.accountName
          )

          if (!accountInfo) {
            throw new Error('Account info not found')
          }

          cache.clearAccountsRegistry()

          return {
            data: accountInfo
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to create an account for ${JSON.stringify(
              args,
              undefined,
              2
            )}`,
            error
          )
        }
      },
      invalidatesTags: [
        'AccountInfos',
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    updateAccountName: mutation<
      true,
      {
        accountId: BraveWallet.AccountId
        name: string
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { keyringService } = api

          const result = await keyringService.setAccountName(
            arg.accountId,
            arg.name
          )

          if (!result.success) {
            throw new Error('Update failed')
          }

          cache.clearAccountsRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to update account name',
            error
          )
        }
      },
      invalidatesTags: ['AccountInfos']
    }),

    importAccount: mutation<
      true,
      {
        accountName: string
        privateKey: string
        coin: BraveWallet.CoinType
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { keyringService } = api
          const result = await keyringService.importAccount(
            arg.accountName,
            arg.privateKey,
            arg.coin
          )

          if (!result.account) {
            throw new Error('No result')
          }

          cache.clearAccountsRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to import account',
            error
          )
        }
      },
      invalidatesTags: [
        'AccountInfos',
        'Network',
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    importFilAccount: mutation<
      true,
      {
        accountName: string
        privateKey: string
        network: FilecoinNetwork
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { keyringService } = api
          const result = await keyringService.importFilecoinAccount(
            arg.accountName,
            arg.privateKey,
            arg.network
          )

          if (!result.account) {
            throw new Error('No result')
          }

          cache.clearAccountsRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to import account',
            error
          )
        }
      },
      invalidatesTags: [
        'AccountInfos',
        'Network',
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    importBtcAccount: mutation<
      true,
      {
        accountName: string
        payload: string
        network: BitcoinNetwork
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { keyringService } = api
          const result = await keyringService.importBitcoinAccount(
            arg.accountName,
            arg.payload,
            arg.network
          )

          if (!result.account) {
            throw new Error('No result')
          }

          cache.clearAccountsRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to import account',
            error
          )
        }
      },
      invalidatesTags: [
        'AccountInfos',
        'Network',
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    importAccountFromJson: mutation<
      true,
      {
        accountName: string
        password: string
        json: string
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { keyringService } = api
          const result = await keyringService.importAccountFromJson(
            arg.accountName,
            arg.password,
            arg.json
          )

          if (!result.account) {
            throw new Error('No result')
          }

          cache.clearAccountsRegistry()

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to import account from JSON',
            error
          )
        }
      },
      invalidatesTags: [
        'AccountInfos',
        'Network',
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance'
      ]
    }),

    importHardwareAccounts: mutation<true, BraveWallet.HardwareWalletAccount[]>(
      {
        queryFn: async (
          hardwareAccounts,
          { endpoint },
          extraOptions,
          baseQuery
        ) => {
          try {
            const { cache, data: api } = baseQuery(undefined)
            api.keyringService.addHardwareAccounts(hardwareAccounts)
            cache.clearAccountsRegistry()
            return {
              data: true
            }
          } catch (error) {
            return handleEndpointError(
              endpoint,
              'Failed to import hardware accounts',
              error
            )
          }
        },
        invalidatesTags: [
          'AccountInfos',
          'TokenBalances',
          'TokenBalancesForChainId',
          'AccountTokenCurrentBalance'
        ]
      }
    ),

    removeAccount: mutation<
      true,
      {
        accountId: BraveWallet.AccountId
        password: string
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { cache, data: api } = baseQuery(undefined)
          const { keyringService } = api
          await keyringService.removeAccount(arg.accountId, arg.password)

          cache.clearAccountsRegistry()
          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to remove account',
            error
          )
        }
      },
      invalidatesTags: ['AccountInfos', 'Network']
    })
  }
}
