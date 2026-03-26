// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import * as React from 'react'
import Label from '@brave/leo/react/label'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../common/locale'
import { isHardwareAccount } from '../../../utils/account-utils'
import { UISelectors } from '../../../common/selectors'
import { PanelActions } from '../../../panel/actions'

// types
import {
  BraveWallet,
  HardwareWalletResponseCodeType,
} from '../../../constants/types'

// hooks
import useInterval from '../../../common/hooks/interval'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'
import {
  useGetPendingSignMessageRequestsQuery,
  useProcessSignMessageRequestMutation,
  useSignMessageHardwareMutation,
} from '../../../common/slices/api.slice'

// components
import { AuthorizeHardwareDeviceIFrame } from '../../shared/authorize-hardware-device/authorize-hardware-device'

// style
import {
  StyledWrapper,
  Title,
  IconWrapper,
  EmptySpace,
} from './connect_hardware_wallet_panel.style'
import { Row, Column } from '../../shared/style'
export interface Props {
  hardwareWalletCode: HardwareWalletResponseCodeType | undefined
}

const onClickInstructions = () => {
  const url = 'https://support.brave.app/hc/en-us/articles/4409309138701'

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
  const dispatch = useAppDispatch()
  const history = useHistory()

  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId,
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
    fromAccount: confirmTransactionAccount,
  } = usePendingTransactions()

  const account = signMessageAccount || confirmTransactionAccount

  // memos
  const isConnected = React.useMemo((): boolean => {
    return (
      hardwareWalletCode !== 'deviceNotConnected'
      && hardwareWalletCode !== 'unauthorized'
    )
  }, [hardwareWalletCode])

  const title = React.useMemo(() => {
    if (!account) {
      return ''
    }

    if (
      hardwareWalletCode === 'deviceBusy'
      || hardwareWalletCode === undefined
    ) {
      return getLocale('braveWalletConnectHardwarePanelConfirmation')
    }

    if (
      hardwareWalletCode === 'deviceNotConnected'
      || hardwareWalletCode === 'unauthorized'
    ) {
      return getLocale('braveWalletConnectHardwarePanelConnect').replace(
        '$1',
        account.name,
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
        request: request,
      }).unwrap()
    } else {
      await processSignMessageRequest({
        approved: true,
        id: request.id,
      }).unwrap()
    }
  }, [
    signMessageAccount,
    processSignMessageRequest,
    request,
    signMessageHardware,
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
    onConfirmTransaction,
  ])

  // Don't poll while the initial sign attempt is still in-flight
  // (hardwareWalletCode is undefined until the first attempt completes).
  // Once a result arrives, immediately retry on connection-state transitions
  // so the UI doesn't wait a full interval after the user takes action.
  const hasResult = hardwareWalletCode !== undefined
  const prevIsConnected = React.useRef(isConnected)
  React.useEffect(() => {
    if (hasResult && prevIsConnected.current !== isConnected) {
      prevIsConnected.current = isConnected
      retryHardwareOperation()
    }
  }, [hasResult, isConnected, retryHardwareOperation])

  // custom hooks
  useInterval(
    retryHardwareOperation,
    hasResult ? 1500 : null,
    hasResult && !isConnected ? 3000 : null,
  )

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
    <StyledWrapper
      width='100%'
      height='100%'
      justifyContent='space-between'
    >
      <Column gap='16px'>
        <Row padding='18px'>
          <Title>{getLocale('braveWalletAddAccountConnect')}</Title>
        </Row>
        <Label color={isConnected ? 'green' : 'red'}>
          <Icon
            slot='icon-before'
            name={isConnected ? 'check-circle-filled' : 'close-circle-filled'}
          />
          {isConnected
            ? getLocale('braveWalletConnectHardwarePanelConnected').replace(
                '$1',
                account.name,
              )
            : getLocale('braveWalletConnectHardwarePanelDisconnected').replace(
                '$1',
                account.name,
              )}
        </Label>
      </Column>
      <Column gap='24px'>
        <IconWrapper padding='16px'>
          <Icon name='flashdrive' />
        </IconWrapper>
        <Column
          padding='16px'
          gap='16px'
        >
          <Title>{title}</Title>
          <Row width='unset'>
            <Button
              kind='plain'
              size='small'
              onClick={onClickInstructions}
            >
              {getLocale('braveWalletConnectHardwarePanelInstructions')}
            </Button>
          </Row>
        </Column>
      </Column>
      <Column
        width='100%'
        padding='16px'
        gap='16px'
      >
        {hardwareWalletCode === 'unauthorized' ? (
          <AuthorizeHardwareDeviceIFrame coinType={account.accountId.coin} />
        ) : (
          <EmptySpace />
        )}
        <Row>
          <Button
            kind='outline'
            size='medium'
            onClick={onCancelConnect}
            isDisabled={hardwareWalletCode === 'deviceBusy'}
          >
            {getLocale('braveWalletButtonCancel')}
          </Button>
        </Row>
      </Column>
    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
