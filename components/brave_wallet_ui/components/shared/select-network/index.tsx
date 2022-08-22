import * as React from 'react'
import { useSelector } from 'react-redux'

import SelectNetworkItem from '../select-network-item'
import { BraveWallet, WalletState } from '../../../constants/types'

interface Props {
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
  selectedNetwork: BraveWallet.NetworkInfo | undefined
}

function SelectNetwork ({
  onSelectCustomNetwork,
  selectedNetwork
}: Props) {
  // redux
  const networks = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)

  return (
    <>
      {networks.map((network) =>
        <SelectNetworkItem
          selectedNetwork={selectedNetwork}
          key={`${network.chainId}-${network.coin}`}
          network={network}
          onSelectCustomNetwork={onSelectCustomNetwork}
        />
      )}
    </>
  )
}

export default SelectNetwork
