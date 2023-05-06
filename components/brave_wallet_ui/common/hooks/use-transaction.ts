// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/query/react'

import { useGetTransactionsQuery } from '../slices/api.slice'

export const useTransactionQuery = (
  txID: string | typeof skipToken,
  opts?: { skip?: boolean }
) => {
  return useGetTransactionsQuery(
    txID === skipToken
      ? skipToken
      : {
          address: null,
          chainId: null,
          coinType: null
        },
    {
      skip: txID === skipToken || opts?.skip,
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        transaction: res.data?.find((tx) => tx.id === txID)
      })
    }
  )
}