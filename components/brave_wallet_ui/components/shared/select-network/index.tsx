import * as React from 'react'
import SelectNetworkItem from '../select-network-item'
import { NetworkOptionsType, Network } from '../../../constants/types'

export interface Props {
  networks: NetworkOptionsType[]
  onSelectNetwork: (network: Network) => () => void
}

function SelectNetwork (props: Props) {
  const { networks, onSelectNetwork } = props
  return <>{networks.map((network) => <SelectNetworkItem key={network.id} network={network} onSelectNetwork={onSelectNetwork(network.id)} />)}</>
}

export default SelectNetwork
