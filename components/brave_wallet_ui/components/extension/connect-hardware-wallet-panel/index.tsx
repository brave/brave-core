// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import * as React from 'react'
import { ThunkDispatch } from '@reduxjs/toolkit'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../common/locale'
import { isHardwareAccount } from '../../../utils/account-utils'
import { UISelectors } from '../../../common/selectors'
import { PanelActions } from '../../../panel/actions'

// types
import {
  BraveWallet,
  HardwareWalletResponseCodeType
} from '../../../constants/types'

// hooks
import useInterval from '../../../common/hooks/interval'
import { useDispatch } from 'react-redux'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import {
  usePendingTransactions //
} from '../../../common/hooks/use-pending-transaction'
import {
  useGetPendingSignMessageRequestsQuery,
  useProcessSignMessageRequestMutation,
  useSignMessageHardwareMutation
} from '../../../common/slices/api.slice'

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
  hardwareWalletCode: HardwareWalletResponseCodeType | undefined
}

const onClickInstructions = () => {
  const url = 'https://support.brave.com/hc/en-us/articles/4409309138701'

  chrome.tabs.create({ url }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
}

function getAppName(coinType: BraveWallet.CoinType): string {
  switch (coinType) {
    case BraveWallet.CoinType.SOL:
      return 'Solana'
    case BraveWallet.CoinType.FIL:
      return 'Filecoin'
    case BraveWallet.CoinType.ETH:
      return 'Ethereum'
    case BraveWallet.CoinType.BTC:
      return 'Bitcoin'
  }

  assertNotReached(`Unsupported coin ${coinType}`)
}

export const ConnectHardwareWalletPanel = ({ hardwareWalletCode }: Props) => {
  // redux
  const dispatch = useDispatch<ThunkDispatch<any, any, any>>()
  const history = useHistory()

  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId
  )

  const isConfirming = !!selectedPendingTransactionId

  // mutations
  const [signMessageHardware] = useSignMessageHardwareMutation()
  const [processSignMessageRequest] = useProcessSignMessageRequestMutation()

  // queries
  const { data: signMessageData } = useGetPendingSignMessageRequestsQuery()
  const request = signMessageData?.at(0)
  const isSigning = request && request.id !== -1

  const { account: signMessageAccount } = useAccountQuery(request?.accountId)

  // pending transactions
  const {
    onConfirm: onConfirmTransaction,
    selectedPendingTransaction,
    fromAccount: confirmTransactionAccount
  } = usePendingTransactions()

  const account = signMessageAccount || confirmTransactionAccount

  // memos
  const isConnected = React.useMemo((): boolean => {
    return (
      hardwareWalletCode !== 'deviceNotConnected' &&
      hardwareWalletCode !== 'unauthorized'
    )
  }, [hardwareWalletCode])

  const title = React.useMemo(() => {
    if (!account) {
      return ''
    }

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

    const network = getAppName(account.accountId.coin)
    return getLocale('braveWalletConnectHardwarePanelOpenApp')
      .replace('$1', network)
      .replace('$2', account.name)
  }, [hardwareWalletCode, account])

  // methods
  const onCancelConnect = React.useCallback(() => {
    if (!account) {
      return
    }

    dispatch(PanelActions.cancelConnectHardwareWallet(account))
  }, [account, dispatch])

  const onSignData = React.useCallback(async () => {
    if (!signMessageAccount || !request) {
      return
    }

    if (isHardwareAccount(signMessageAccount.accountId)) {
      await signMessageHardware({
        account: signMessageAccount,
        request: request
      }).unwrap()
    } else {
      await processSignMessageRequest({
        approved: true,
        id: request.id
      }).unwrap()
    }
  }, [
    signMessageAccount,
    processSignMessageRequest,
    request,
    signMessageHardware
  ])

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

  React.useEffect(() => {
    // After Panel V2 this is needed to reset the origin
    // back to brave://wallet-panel.top-chrome/ without any
    // params. Otherwise hardware authorization will fail.
    history.push('')
  }, [history])

  // render

  if (!account) {
    return null
  }
  return (
    <StyledWrapper>
      <ConnectionRow>
        <Indicator isConnected={isConnected} />
        <Description>
          {isConnected
            ? getLocale('braveWalletConnectHardwarePanelConnected').replace(
                '$1',
                account.name
              )
            : getLocale('braveWalletConnectHardwarePanelDisconnected').replace(
                '$1',
                account.name
              )}
        </Description>
      </ConnectionRow>
      <Title>{title}</Title>
      <InstructionsButton onClick={onClickInstructions}>
        {getLocale('braveWalletConnectHardwarePanelInstructions')}
      </InstructionsButton>
      <PageIcon />
      {hardwareWalletCode !== 'deviceBusy' &&
        (hardwareWalletCode === 'unauthorized' ? (
          <AuthorizeHardwareDeviceIFrame coinType={account.accountId.coin} />
        ) : (
          <ButtonWrapper>
            <NavButton
              buttonType='secondary'
              text={getLocale('braveWalletButtonCancel')}
              onSubmit={onCancelConnect}
            />
          </ButtonWrapper>
        ))}
    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
