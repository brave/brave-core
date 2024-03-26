// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export const encryptionEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    getPendingDecryptRequest: query<BraveWallet.DecryptRequest | null, void>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          const { requests } =
            await api.braveWalletService.getPendingDecryptRequests()

          return {
            data: requests.length ? requests[0] : null
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to get pending decrypt request',
            error
          )
        }
      },
      providesTags: ['PendingDecryptRequest']
    }),

    processPendingDecryptRequest: mutation<
      boolean,
      {
        requestId: string
        approved: boolean
      }
    >({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)

          api.braveWalletService.notifyDecryptRequestProcessed(
            arg.requestId,
            arg.approved
          )

          const { requests } =
            await api.braveWalletService.getPendingDecryptRequests()

          if (!requests.length) {
            api.panelHandler?.closeUI()
          }

          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Failed to process pending decrypt request',
            error
          )
        }
      },
      invalidatesTags: ['PendingDecryptRequest']
    })
  }
}
