import * as React from 'react'

import Registry, { SwapExchangeProxy } from './registry'
import { WalletAccountType } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'

export default function useAddressLabels (accounts: WalletAccountType[]) {
  const findAccountName = React.useCallback((address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }, [accounts])

  return (address: string): string =>
    Registry[address.toLowerCase()] ?? findAccountName(address) ?? reduceAddress(address)
}

export { SwapExchangeProxy }
