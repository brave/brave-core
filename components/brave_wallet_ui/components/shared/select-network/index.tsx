import * as React from 'react'
import { useSelector } from 'react-redux'

import SelectNetworkItem from '../select-network-item'
import { BraveWallet, WalletState } from '../../../constants/types'

interface Props {
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
  selectedNetwork?: BraveWallet.NetworkInfo
}

function SelectNetwork ({
  onSelectCustomNetwork,
  selectedNetwork
}: Props) {
  // redux
  const {
    networkList: networks,
    selectedNetwork: reduxSelectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  return (
    <>
      {networks.map((network) =>
        <SelectNetworkItem
          selectedNetwork={selectedNetwork || reduxSelectedNetwork}
          key={`${network.chainId}-${network.coin}`}
          network={network}
          onSelectCustomNetwork={onSelectCustomNetwork}
        />
      )}
    </>
  )
}

export default SelectNetwork
