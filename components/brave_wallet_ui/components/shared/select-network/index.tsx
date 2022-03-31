import * as React from 'react'
import SelectNetworkItem from '../select-network-item'
import { BraveWallet } from '../../../constants/types'

export interface Props {
  networks: BraveWallet.NetworkInfo[]
  selectedNetwork: BraveWallet.NetworkInfo
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => () => void
}

function SelectNetwork (props: Props) {
  const { networks, onSelectNetwork, selectedNetwork } = props

  // MULTICHAIN: Remove me once we support SOL and FIL transaction creation.
  // Will be implemented in these 2 issues
  // https://github.com/brave/brave-browser/issues/20698
  // https://github.com/brave/brave-browser/issues/20893
  const networkList = React.useMemo(() => {
    return networks.filter(
      (network) =>
        network.coin !== BraveWallet.CoinType.SOL &&
        network.coin !== BraveWallet.CoinType.FIL
    )
  }, [networks])

  return (
    <>
      {networkList.map((network) =>
        <SelectNetworkItem
          selectedNetwork={selectedNetwork}
          key={`${network.chainId}-${network.coin}`}
          network={network}
          onSelectNetwork={onSelectNetwork(network)}
        />
      )}
    </>
  )
}

export default SelectNetwork
