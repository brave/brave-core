// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ThunkDispatch } from '@reduxjs/toolkit'

// utils
import { getLocale } from '../../../../common/locale'
import { isHardwareAccount } from '../../../utils/account-utils'
import { PanelSelectors } from '../../../panel/selectors'
import { UISelectors } from '../../../common/selectors'
import { PanelActions } from '../../../panel/actions'

// types
import { BraveWallet } from '../../../constants/types'
import { HardwareWalletResponseCodeType } from '../../../common/hardware/types'

// hooks
import useInterval from '../../../common/hooks/interval'
import { useDispatch } from 'react-redux'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'
import {
  useSafeUISelector,
  useUnsafePanelSelector
} from '../../../common/hooks/use-safe-selector'
import {
  usePendingTransactions //
} from '../../../common/hooks/use-pending-transaction'

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
  onCancel: (account: BraveWallet.AccountInfo) => void
  account: BraveWallet.AccountInfo
  hardwareWalletCode: HardwareWalletResponseCodeType | undefined
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
  account,
  hardwareWalletCode,
  onClickInstructions
}: Props) => {
  // redux
  const dispatch = useDispatch<ThunkDispatch<any, any, any>>()

  /**
   * signMessageData by default initialized as:
   *
   * ```[{ id: -1, address: '', message: '' }]```
   */
  const signMessageData = useUnsafePanelSelector(PanelSelectors.signMessageData)
  const request = signMessageData.at(0)
  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId
  )
  const isSigning = request && request.id !== -1

  const isConfirming = !!selectedPendingTransactionId
  const coinType = account.accountId.coin

  // queries
  const { account: messageAccount } = useAccountQuery(request?.accountId)

  // pending transactions
  const { onConfirm: onConfirmTransaction, selectedPendingTransaction } =
    usePendingTransactions()

  // memos
  const isConnected = React.useMemo((): boolean => {
    return (
      hardwareWalletCode !== 'deviceNotConnected' &&
      hardwareWalletCode !== 'unauthorized'
    )
  }, [hardwareWalletCode])

  const title = React.useMemo(() => {
    if (hardwareWalletCode === 'deviceBusy') {
      return getLocale('braveWalletConnectHardwarePanelConfirmation')
    }

    // Not connected
    if (
      hardwareWalletCode === 'deviceNotConnected' ||
      hardwareWalletCode === 'unauthorized'
    ) {
      return getLocale('braveWalletConnectHardwarePanelConnect').replace(
        '$1',
        account.name
      )
    }

    const network = getAppName(coinType)
    return getLocale('braveWalletConnectHardwarePanelOpenApp')
      .replace('$1', network)
      .replace('$2', account.name)
  }, [hardwareWalletCode, coinType, account.name])

  // methods
  const onCancelConnect = React.useCallback(() => {
    onCancel(account)
  }, [onCancel, account])

  const onSignData = React.useCallback(() => {
    if (!messageAccount || !request) {
      return
    }

    if (isHardwareAccount(messageAccount.accountId)) {
      dispatch(
        PanelActions.signMessageHardware({
          account: messageAccount,
          request: request
        })
      )
    } else {
      dispatch(
        PanelActions.signMessageProcessed({
          approved: true,
          id: request.id
        })
      )
    }
  }, [messageAccount, request])

  const retryHardwareOperation = React.useCallback(() => {
    if (isSigning) {
      onSignData()
    }
    if (isConfirming && selectedPendingTransaction) {
      onConfirmTransaction()
    }
  }, [
    isSigning,
    isConfirming,
    selectedPendingTransaction,
    onSignData,
    onConfirmTransaction
  ])

  // custom hooks
  useInterval(retryHardwareOperation, 3000, !isConnected ? 5000 : null)

  // render
  return (
    <StyledWrapper>
      <ConnectionRow>
        <Indicator isConnected={isConnected} />
        <Description>
          {
            isConnected
              ? getLocale('braveWalletConnectHardwarePanelConnected').replace('$1', account.name)
              : getLocale('braveWalletConnectHardwarePanelDisconnected').replace('$1', account.name)
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
