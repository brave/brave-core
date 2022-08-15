// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { create } from 'ethereum-blockies'

// types
import { BuySendSwapViewTypes, WalletState } from '../../../constants/types'

// utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'

// components
import Tooltip from '../../shared/tooltip/index'
import SelectNetworkButton from '../../shared/select-network-button/index'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'

// Styled Components
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  NameAndIcon,
  SwitchIcon
} from './style'

export interface Props {
  onChangeSwapView: (view: BuySendSwapViewTypes) => void
}

export const SwapHeader = ({ onChangeSwapView }: Props) => {
  // redux
  const selectedAccount = useSelector(({ wallet }: {wallet: WalletState}) => wallet.selectedAccount)
  const selectedNetwork = useSelector(({ wallet }: {wallet: WalletState}) => wallet.selectedNetwork)

  // methods
  const onShowAccounts = React.useCallback(() => {
    onChangeSwapView('acounts')
  }, [onChangeSwapView])

  const onShowNetworks = React.useCallback(() => {
    onChangeSwapView('networks')
  }, [onChangeSwapView])

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount])

  // render
  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle onClick={onShowAccounts} orb={orb}>
          <SwitchIcon />
        </AccountCircle>
        <CopyTooltip text={selectedAccount.address}>
          <AccountAndAddress>
            <AccountName>{reduceAccountDisplayName(selectedAccount.name, 11)}</AccountName>
            <AccountAddress>{reduceAddress(selectedAccount.address)}</AccountAddress>
          </AccountAndAddress>
        </CopyTooltip>
      </NameAndIcon>
      <Tooltip text={selectedNetwork.chainName}>
        <SelectNetworkButton
          selectedNetwork={selectedNetwork}
          onClick={onShowNetworks}
        />
      </Tooltip>
    </StyledWrapper >
  )
}

export default SwapHeader
