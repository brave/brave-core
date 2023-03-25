// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'
import { getNetworkFromTXDataUnion } from '../../utils/network-utils'
import { useGetDefaultNetworksQuery, useGetSelectedChainQuery } from '../slices/api.slice'

export const useTransactionsNetwork = <
 T extends SerializableTransactionInfo | BraveWallet.TransactionInfo
>(transaction: T) => {
  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: defaultNetworks = [] } = useGetDefaultNetworksQuery()

  const txNetwork = React.useMemo(() => {
    return getNetworkFromTXDataUnion(transaction.txDataUnion, defaultNetworks, selectedNetwork)
  }, [defaultNetworks, transaction, selectedNetwork])

  return txNetwork
}
