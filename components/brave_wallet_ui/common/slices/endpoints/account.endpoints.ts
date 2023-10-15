// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// utils
import { handleEndpointError } from '../../../utils/api-utils'

export const accountEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
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
          const { accountInfo } = await baseQuery(
            undefined
          ).data.keyringService.addAccount(
            args.coin,
            args.keyringId,
            args.accountName
          )

          if (!accountInfo) {
            throw new Error('Account info not found')
          }

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
      invalidatesTags: ['AccountInfos']
    })
  }
}
