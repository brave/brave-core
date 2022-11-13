// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// types
import { BraveWallet } from '../../../constants/types'
import { HardwareWalletResponseCodeType } from '../../../common/hardware/types'

// hooks
import useInterval from '../../../common/hooks/interval'

// components
import { NavButton } from '../buttons/nav-button/index'
import { AuthorizeHardwareDeviceIFrame } from '../../shared/authorize-hardware-device/authorize-hardware-device'

// style
import {
  StyledWrapper,
  Title,
  Description,
  PageIcon,
  InstructionsButton,
  ButtonWrapper,
  Indicator,
  ConnectionRow
} from './style'
export interface Props {
  onCancel: (accountAddress: string, coinType: BraveWallet.CoinType) => void
  walletName: string
  accountAddress: string
  coinType: BraveWallet.CoinType
  hardwareWalletCode: HardwareWalletResponseCodeType | undefined
  retryCallable: () => void
  onClickInstructions: () => void
}

function getAppName (coinType: BraveWallet.CoinType): string {
  switch (coinType) {
    case BraveWallet.CoinType.SOL:
      return 'Solana'
    case BraveWallet.CoinType.FIL:
      return 'Filecoin'
    default:
      return 'Ethereum'
  }
}

export const ConnectHardwareWalletPanel = ({
  onCancel,
  walletName,
  accountAddress,
  coinType,
  hardwareWalletCode,
  retryCallable,
  onClickInstructions
}: Props) => {
  // memos
  const isConnected = React.useMemo((): boolean => {
    return hardwareWalletCode !== 'deviceNotConnected' && hardwareWalletCode !== 'unauthorized'
  }, [hardwareWalletCode])

  const title = React.useMemo(() => {
    if (hardwareWalletCode === 'deviceBusy') {
      return getLocale('braveWalletConnectHardwarePanelConfirmation')
    }

    // Not connected
    if (hardwareWalletCode === 'deviceNotConnected' || hardwareWalletCode === 'unauthorized') {
      return getLocale('braveWalletConnectHardwarePanelConnect').replace('$1', walletName)
    }

    const network = getAppName(coinType)
    return getLocale('braveWalletConnectHardwarePanelOpenApp')
      .replace('$1', network)
      .replace('$2', walletName)
  }, [hardwareWalletCode])

  // custom hooks
  useInterval(retryCallable, 3000, !isConnected ? 5000 : null)

  // methods
  const onCancelConnect = React.useCallback(() => {
    onCancel(accountAddress, coinType)
  }, [onCancel, accountAddress, coinType])

  // render
  return (
    <StyledWrapper>
      <ConnectionRow>
        <Indicator isConnected={isConnected} />
        <Description>
          {
            isConnected
              ? getLocale('braveWalletConnectHardwarePanelConnected').replace('$1', walletName)
              : getLocale('braveWalletConnectHardwarePanelDisconnected').replace('$1', walletName)
          }
        </Description>
      </ConnectionRow>
      <Title>{title}</Title>
      <InstructionsButton onClick={onClickInstructions}>{getLocale('braveWalletConnectHardwarePanelInstructions')}</InstructionsButton>
      <PageIcon />
      {
        hardwareWalletCode !== 'deviceBusy' && (hardwareWalletCode === 'unauthorized'
          ? <AuthorizeHardwareDeviceIFrame coinType={coinType}/>
          : <ButtonWrapper>
              <NavButton
                buttonType='secondary'
                text={getLocale('braveWalletButtonCancel')}
                onSubmit={onCancelConnect}
              />
            </ButtonWrapper>
        )
      }
    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
