// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/query/react'
import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { useGetNetworkQuery } from '../slices/api.slice'

export const useTransactionsNetwork = <
  T extends
    | Pick<
        SerializableTransactionInfo | BraveWallet.TransactionInfo,
        'chainId' | 'txDataUnion'
      >
    | undefined
>(
  transaction: T
) => {
  // queries
  const { data: txNetwork } = useGetNetworkQuery(
    transaction
      ? {
          chainId: transaction.chainId,
          coin: getCoinFromTxDataUnion(transaction.txDataUnion)
        }
      : skipToken
  )

  return txNetwork
}
