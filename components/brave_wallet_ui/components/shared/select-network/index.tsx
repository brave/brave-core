import * as React from 'react'
import SelectNetworkItem from '../select-network-item'
import { EthereumChain } from '../../../constants/types'

export interface Props {
  networks: EthereumChain[]
  onSelectNetwork: (network: EthereumChain) => () => void
}

function SelectNetwork (props: Props) {
  const { networks, onSelectNetwork } = props
  return <>{networks.map((network) => <SelectNetworkItem key={network.chainId} network={network} onSelectNetwork={onSelectNetwork(network)} />)}</>
}

export default SelectNetwork
