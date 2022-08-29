// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet, WalletAccountType } from '../../constants/types'

// utils
import { getBalance } from '../../utils/balance-utils'

export const useBalance = (networks: BraveWallet.NetworkInfo[]) => {
  const _getBalance = React.useCallback((account?: WalletAccountType, token?: BraveWallet.BlockchainToken) => {
    return getBalance(networks, account, token)
  }, [networks])

  return _getBalance
}

export default useBalance
