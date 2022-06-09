// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import Registry, { SwapExchangeProxy } from './registry'
import { WalletAccountType } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'

export const useAddressLabels = (accounts: WalletAccountType[]) => {
  const findAccountName = React.useCallback((address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }, [accounts])

  const getAddressLabel = React.useCallback((address: string): string => {
    return (
      Registry[address.toLowerCase()] ??
      findAccountName(address) ??
      reduceAddress(address)
    )
  }, [findAccountName])

  return {
    findAccountName,
    getAddressLabel
  }
}

export { SwapExchangeProxy }
export default useAddressLabels
