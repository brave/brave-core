// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { useGetNetworkQuery } from '../slices/api.slice'

export const useTransactionsNetwork = <
 T extends SerializableTransactionInfo | BraveWallet.TransactionInfo
>(transaction: T) => {
  // queries
  const { data: txNetwork } = useGetNetworkQuery({
    chainId: transaction.chainId,
    coin: getCoinFromTxDataUnion(transaction.txDataUnion)
  })

  return txNetwork
}
