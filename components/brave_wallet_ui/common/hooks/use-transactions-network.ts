// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useSelector } from 'react-redux'
import { BraveWallet, WalletState } from '../../constants/types'
import { getNetworkFromTXDataUnion } from '../../utils/network-utils'

export const useTransactionsNetwork = (transaction: BraveWallet.TransactionInfo) => {
  // redux
  const {
    defaultNetworks, selectedNetwork
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const txNetwork = React.useMemo(() => {
    return getNetworkFromTXDataUnion(transaction.txDataUnion, defaultNetworks, selectedNetwork)
  }, [defaultNetworks, transaction, selectedNetwork])

  return txNetwork
}
