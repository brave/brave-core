// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'
import { handleEndpointError } from '../../../utils/api-utils'

export const snapsEndpoints = ({
  mutation,
  query,
}: WalletApiEndpointBuilderParams) => {
  return {
    getPendingSnapInstall: query<BraveWallet.PendingSnapInstall, void>({
      queryFn: async (_, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { pending } =
            await api.braveWalletService.getPendingSnapInstall()
          return { data: pending }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending snap install state',
            error,
          )
        }
      },
      providesTags: ['PendingSnapInstall'],
    }),

    requestInstallSnap: mutation<
      { success: boolean; error?: string },
      { snapId: string; version: string }
    >({
      queryFn: async (
        { snapId, version },
        { endpoint },
        _extraOptions,
        baseQuery,
      ) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { success, error } =
            await api.braveWalletService.requestInstallSnap(snapId, version)
          return { data: { success, error: error ?? undefined } }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to request snap install',
            error,
          )
        }
      },
      invalidatesTags: ['PendingSnapInstall'],
    }),

    notifySnapInstallRequestProcessed: mutation<void, { approved: boolean }>({
      queryFn: async (
        { approved },
        { endpoint },
        _extraOptions,
        baseQuery,
      ) => {
        try {
          const { data: api } = baseQuery(undefined)
          await api.braveWalletService.notifySnapInstallRequestProcessed(
            approved,
          )
          return { data: undefined }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to notify snap install processed',
            error,
          )
        }
      },
      invalidatesTags: ['PendingSnapInstall', 'InstalledSnaps'],
    }),

    getInstalledSnaps: query<BraveWallet.InstalledSnapInfo[], void>({
      queryFn: async (_, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { snaps } = await api.braveWalletService.getInstalledSnaps()
          return { data: snaps }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get installed snaps',
            error,
          )
        }
      },
      providesTags: ['InstalledSnaps'],
    }),

    getSnapHomePage: query<
      { contentJson: string | null; interfaceId: string | null; error: string | null },
      { snapId: string }
    >({
      queryFn: async ({ snapId }, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { contentJson, interfaceId, error } =
            await api.braveWalletService.getSnapHomePage(snapId)
          return { data: { contentJson: contentJson ?? null, interfaceId: interfaceId ?? null, error: error ?? null } }
        } catch (error) {
          return handleEndpointError(endpoint, 'Failed to get snap home page', error)
        }
      },
    }),

    sendSnapUserInput: mutation<
      { contentJson: string | null; error: string | null },
      { snapId: string; interfaceId: string; eventJson: string }
    >({
      queryFn: async ({ snapId, interfaceId, eventJson }, { endpoint }, _extra, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { contentJson, error } =
            await api.braveWalletService.sendSnapUserInput(snapId, interfaceId, eventJson)
          return { data: { contentJson: contentJson ?? null, error: error ?? null } }
        } catch (error) {
          return handleEndpointError(endpoint, 'Failed to send snap user input', error)
        }
      },
    }),

    uninstallSnap: mutation<void, { snapId: string }>({
      queryFn: async (
        { snapId },
        { endpoint },
        _extraOptions,
        baseQuery,
      ) => {
        try {
          const { data: api } = baseQuery(undefined)
          await api.braveWalletService.uninstallSnap(snapId)
          return { data: undefined }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to uninstall snap',
            error,
          )
        }
      },
      invalidatesTags: ['InstalledSnaps'],
    }),
  }
}
