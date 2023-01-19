// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet, PanelTypes } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'

// components
import { Tooltip } from '../../shared'

// style
import {
  StyledWrapper,
  TransactionsIcon,
  NavButton,
  NavButtonText,
  NavDivider,
  NavOutline,
  TransactionsButton
} from './style'

export interface Props {
  onNavigate: (path: PanelTypes) => void
  isSwapDisabled: boolean
  isBuyDisabled: boolean
  selectedNetwork?: BraveWallet.NetworkInfo
}

function ConnectedBottomNav (props: Props) {
  const { onNavigate, isSwapDisabled, isBuyDisabled, selectedNetwork } = props

  const navigate = (path: PanelTypes) => () => {
    onNavigate(path)
  }

  return (
    <StyledWrapper>
      <NavOutline>
        <Tooltip
          isVisible={isBuyDisabled}
          position='right'
          verticalPosition='above'
          maxWidth='150px'
          text={
            selectedNetwork
              ? getLocale('braveWalletBuyNotSupportedTooltip').replace('$1', selectedNetwork.chainName)
              : ''
          }
        >
          <NavButton disabled={isBuyDisabled} onClick={navigate('buy')}>
            <NavButtonText disabled={isBuyDisabled}>{getLocale('braveWalletBuy')}</NavButtonText>
          </NavButton>
        </Tooltip>

        <NavDivider />

        <NavButton onClick={navigate('send')}>

          <NavButtonText>{getLocale('braveWalletSend')}</NavButtonText>
        </NavButton>

        <NavDivider />

        <Tooltip
          isVisible={isSwapDisabled}
          position='left'
          verticalPosition='above'
          text={
            selectedNetwork
              ? getLocale('braveWalletSwapNotSupportedTooltip').replace('$1', selectedNetwork.chainName)
              : ''
          }
          maxWidth='150px'
        >
          <NavButton disabled={isSwapDisabled} onClick={navigate('swap')}>
            <NavButtonText disabled={isSwapDisabled}>{getLocale('braveWalletSwap')}</NavButtonText>
          </NavButton>
        </Tooltip>

        <NavDivider />

        <TransactionsButton onClick={navigate('activity')}>
          <TransactionsIcon />
        </TransactionsButton>

      </NavOutline>
    </StyledWrapper>
  )
}

export default ConnectedBottomNav
