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
