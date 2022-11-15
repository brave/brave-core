// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

import { SelectNetwork } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollContainer
} from '../shared-styles'

import { WalletActions } from '../../../common/actions'
import { BraveWallet, WalletState } from '../../../constants/types'

export interface Props {
  hasAddButton?: boolean
  onBack: () => void
  onAddNetwork?: () => void
}

export const SelectNetworkWithHeader = ({
  onBack,
  hasAddButton,
  onAddNetwork
}: Props) => {
  // redux
  const dispatch = useDispatch()
  const selectedNetwork = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetwork)

  // methods
  const onSelectCustomNetwork = React.useCallback((network: BraveWallet.NetworkInfo): void => {
    dispatch(WalletActions.selectNetwork(network))
    onBack()
  }, [onBack])

  // render
  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectNetwork')}
        onBack={onBack}
        hasAddButton={hasAddButton}
        onClickAdd={onAddNetwork}
      />
      <SelectScrollContainer>
        <SelectNetwork
          onSelectCustomNetwork={onSelectCustomNetwork}
          selectedNetwork={selectedNetwork}
        />
      </SelectScrollContainer>
    </SelectWrapper>
  )
}

export default SelectNetworkWithHeader
