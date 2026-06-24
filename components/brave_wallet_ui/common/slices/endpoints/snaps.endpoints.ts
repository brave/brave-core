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
            await api.snapsService.getPendingSnapInstall()
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
        console.error('XXXZZZ requestInstallSnap: snapId=' + snapId + ' version=' + version)
        try {
          const { data: api } = baseQuery(undefined)
          const { success, error } =
            await api.snapsService.requestInstallSnap(snapId, version)
          console.error('XXXZZZ requestInstallSnap: result success=' + success + ' error=' + error)
          return { data: { success, error: error ?? undefined } }
        } catch (error) {
          console.error('XXXZZZ requestInstallSnap: threw', error)
          return handleEndpointError(
            endpoint,
            'Failed to request snap install',
            error,
          )
        }
      },
      invalidatesTags: ['PendingSnapInstall'],
    }),

    notifySnapInstallRequestProcessed: mutation<null, { approved: boolean }>({
      queryFn: async (
        { approved },
        { endpoint },
        _extraOptions,
        baseQuery,
      ) => {
        console.error('XXXZZZ notifySnapInstallRequestProcessed: approved=' + approved)
        try {
          const { data: api } = baseQuery(undefined)
          const mojoResult = await api.snapsService.notifySnapInstallRequestProcessed(
            approved,
          )
          console.error('XXXZZZ notifySnapInstallRequestProcessed: mojo returned', JSON.stringify(mojoResult))
          return { data: null }
        } catch (error) {
          console.error('XXXZZZ notifySnapInstallRequestProcessed: threw', error)
          return handleEndpointError(
            endpoint,
            'Failed to notify snap install processed',
            error,
          )
        }
      },
      invalidatesTags: ['PendingSnapInstall', 'InstalledSnaps'],
    }),

    getInstalledSnaps: query<BraveWallet.SnapInstallData[], void>({
      queryFn: async (_, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { snaps } = await api.snapsService.getInstalledSnaps()
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
            await api.snapsService.getSnapHomePage(snapId)
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
            await api.snapsService.sendSnapUserInput(snapId, interfaceId, eventJson)
          return { data: { contentJson: contentJson ?? null, error: error ?? null } }
        } catch (error) {
          return handleEndpointError(endpoint, 'Failed to send snap user input', error)
        }
      },
    }),

    getPendingSnapConnection: query<
      BraveWallet.PendingSnapConnection | null,
      void
    >({
      queryFn: async (_, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { pending } =
            await api.snapsService.getPendingSnapConnection()
          return { data: pending ?? null }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending snap connection',
            error,
          )
        }
      },
      providesTags: ['PendingSnapConnection'],
    }),

    notifySnapConnectionRequestProcessed: mutation<
      null,
      { approved: boolean }
    >({
      queryFn: async ({ approved }, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          await api.snapsService.notifySnapConnectionRequestProcessed(approved)
          return { data: null }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to notify snap connection processed',
            error,
          )
        }
      },
      invalidatesTags: ['PendingSnapConnection'],
    }),

    getSnapConnectedOrigins: query<string[], { snapId: string }>({
      queryFn: async ({ snapId }, { endpoint }, _extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          const { origins } =
            await api.snapsService.getConnectedOrigins(snapId)
          return { data: origins }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get snap connected origins',
            error,
          )
        }
      },
      providesTags: (_result, _error, { snapId }) => [
        { type: 'SnapConnectedOrigins', id: snapId },
      ],
    }),

    disconnectSnapOrigin: mutation<
      null,
      { origin: string; snapId: string }
    >({
      queryFn: async (
        { origin, snapId },
        { endpoint },
        _extraOptions,
        baseQuery,
      ) => {
        try {
          const { data: api } = baseQuery(undefined)
          await api.snapsService.disconnectSnapOrigin(origin, snapId)
          return { data: null }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to disconnect snap origin',
            error,
          )
        }
      },
      invalidatesTags: (_result, _error, { snapId }) => [
        { type: 'SnapConnectedOrigins', id: snapId },
      ],
    }),

    uninstallSnap: mutation<null, { snapId: string }>({
      queryFn: async (
        { snapId },
        { endpoint },
        _extraOptions,
        baseQuery,
      ) => {
        try {
          const { data: api } = baseQuery(undefined)
          await api.snapsService.uninstallSnap(snapId)
          return { data: null }
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
