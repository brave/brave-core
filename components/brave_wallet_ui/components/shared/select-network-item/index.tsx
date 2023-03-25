// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch } from 'react-redux'

// Constants
import { BraveWallet } from '../../../constants/types'
import { AllNetworksOption } from '../../../options/network-filter-options'
import { PanelActions } from '../../../panel/actions'

// Hooks
import { useSetNetworkMutation } from '../../../common/slices/api.slice'

// Components
import { CreateNetworkIcon } from '../'

// Styled Components
import {
  StyledWrapper,
  NetworkName,
  LeftSide,
  BigCheckMark
} from './style'


export interface Props {
  selectedNetwork?: BraveWallet.NetworkInfo
  network: BraveWallet.NetworkInfo
  onSelectCustomNetwork?: (network: BraveWallet.NetworkInfo) => void
}

function SelectNetworkItem (props: Props) {
  const { network, selectedNetwork, onSelectCustomNetwork } = props

  // redux
  const dispatch = useDispatch()

  // queries & mutations
  const [setNetwork] = useSetNetworkMutation()


  // methods
  const onSelectNetwork = React.useCallback(async () => {
    if (onSelectCustomNetwork) {
      onSelectCustomNetwork(network)
      return
    }

    await setNetwork({
      chainId: network.chainId,
      coin: network.coin
    })

    dispatch(PanelActions.navigateTo('main'))
  }, [onSelectCustomNetwork, setNetwork])

  // render
  return (
    <StyledWrapper onClick={onSelectNetwork} data-test-chain-id={'chain-' + network.chainId}>
      <LeftSide>
        {network.chainId !== AllNetworksOption.chainId &&
          <CreateNetworkIcon network={network} marginRight={14} />
        }
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
