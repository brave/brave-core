import * as React from 'react'
import SelectNetworkItem from '../select-network-item'
import { NetworkOptionsType } from '../../../constants/types'

export interface Props {
  networks: NetworkOptionsType[]
  onSelectNetwork: (network: NetworkOptionsType) => () => void
}

function SelectNetwork (props: Props) {
  const { networks, onSelectNetwork } = props
  return <>{networks.map((network) => <SelectNetworkItem key={network.id} network={network} onSelectNetwork={onSelectNetwork(network)} />)}</>
}

export default SelectNetwork
