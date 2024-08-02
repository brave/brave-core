// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import {
  getHasPendingRequests,
  handleEndpointError
} from '../../../utils/api-utils'
import { getEntitiesListFromEntityState } from '../../../utils/entities.utils'
import { storeCurrentAndPreviousPanel } from '../../../utils/local-storage-utils'

export const sitePermissionEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getActiveOriginConnectedAccountIds: query<BraveWallet.AccountId[], void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api, cache } = baseQuery(undefined)
          const { braveWalletService } = api

          const accountsRegistry = await cache.getAccountsRegistry()
          const accounts = getEntitiesListFromEntityState(accountsRegistry)

          // Get a list of accounts with permissions of the active origin
          const { accountsWithPermission } =
            await braveWalletService.hasPermission(
              accounts.map((acc) => acc.accountId)
            )

          return {
            data: accountsWithPermission
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get account with permission for the active origin',
            error
          )
        }
      },
      providesTags: ['ConnectedAccounts']
    }),

    getIsPrivateWindow: query<boolean, void>({
      queryFn: async (_, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { braveWalletService } = api

          const { isPrivateWindow } = await braveWalletService.isPrivateWindow()

          return {
            data: isPrivateWindow
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get private window status',
            error
          )
        }
      },
      providesTags: ['IsPrivateWindow']
    }),

    connectToSite: mutation<
      true,
      {
        addressToConnect: string
        duration: BraveWallet.PermissionLifetimeOption
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { panelHandler } = api

          if (panelHandler) {
            panelHandler.connectToSite([arg.addressToConnect], arg.duration)

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
            `Failed to connect to site`,
            error
          )
        }
      },
      invalidatesTags: ['ConnectedAccounts']
    }),

    cancelConnectToSite: mutation<true, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { panelHandler } = api

          if (panelHandler) {
            storeCurrentAndPreviousPanel('main', undefined)
            panelHandler.cancelConnectToSite()

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
            `Failed to cancel site connection`,
            error
          )
        }
      },
      invalidatesTags: ['ConnectedAccounts']
    }),

    requestSitePermission: mutation<true, BraveWallet.AccountId>({
      queryFn: async (accountId, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { panelHandler } = api

          if (panelHandler) {
            await panelHandler.requestPermission(accountId)
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to request account (${
              accountId //
            }) permission for the active origin`,
            error
          )
        }
      },
      invalidatesTags: ['ConnectedAccounts']
    }),

    removeSitePermission: mutation<true, BraveWallet.AccountId>({
      queryFn: async (accountId, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { braveWalletService } = api

          await braveWalletService.resetPermission(accountId)

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            `Failed to remove account (${
              accountId //
            }) permission for the active origin`,
            error
          )
        }
      },
      invalidatesTags: ['ConnectedAccounts']
    })
  }
}
