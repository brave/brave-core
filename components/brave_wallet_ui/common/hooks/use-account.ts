// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/dist/query'

import { useGetAccountInfosRegistryQuery } from '../slices/api.slice'
import { findAccountFromRegistry } from '../../utils/account-utils'

export const useAccountQuery = (
  address: string | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  return useGetAccountInfosRegistryQuery(
    address === skipToken ? skipToken : undefined,
    {
      skip: address === skipToken || opts?.skip,
      selectFromResult: (res) => ({
        isLoading: res?.isLoading,
        error: res?.error,
        data: res.data,
        account:
          res.data && address !== skipToken
            ? findAccountFromRegistry(address, res.data)
            : undefined
      })
    }
  )
}
