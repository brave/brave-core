import * as React from 'react'
import { useDispatch } from 'react-redux'
import { BraveWallet } from '../../../constants/types'
import { CreateNetworkIcon } from '../'
// Styled Components
import {
  StyledWrapper,
  NetworkName,
  LeftSide,
  BigCheckMark
} from './style'
import { WalletActions } from '../../../common/actions'
import { PanelActions } from '../../../panel/actions'

export interface Props {
  selectedNetwork?: BraveWallet.NetworkInfo
  network: BraveWallet.NetworkInfo
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
}

function SelectNetworkItem (props: Props) {
  const { network, selectedNetwork, onSelectCustomNetwork } = props

  // redux
  const dispatch = useDispatch()

  // methods
  const onSelectNetwork = () => {
    if (onSelectCustomNetwork) {
      onSelectCustomNetwork(network)
      return
    }
    dispatch(WalletActions.selectNetwork(network))
    dispatch(PanelActions.navigateTo('main'))
  }

  // render
  return (
    <StyledWrapper onClick={onSelectNetwork} data-test-chain-id={'chain-' + network.chainId}>
      <LeftSide>
        <CreateNetworkIcon network={network} marginRight={14} />
        <NetworkName>{network.chainName}</NetworkName>
      </LeftSide>
      {
        selectedNetwork?.chainId === network.chainId &&
        selectedNetwork?.coin === network.coin &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectNetworkItem
