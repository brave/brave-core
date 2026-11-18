// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import * as React from 'react'
import Label from '@brave/nala/react/label'
import Icon from '@brave/nala/react/icon'
import Button from '@brave/nala/react/button'

import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../common/locale'
import { isHardwareAccount } from '../../../utils/account-utils'
import { PanelActions } from '../../../panel/actions'
import { PanelSelectors } from '$wallet/panel/selectors'

// types
import {
  BraveWallet,
  HardwareWalletResponseCodeType,
} from '../../../constants/types'

// hooks
import useInterval from '../../../common/hooks/interval'
import { useAppDispatch } from '../../../common/hooks/use-redux'
import {
  useAccountFromAddressQuery,
  useAccountQuery,
} from '$wallet/common/slices/api.slice.extra'
import { useUnsafePanelSelector } from '$wallet/common/hooks/use-safe-selector'
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'
import {
  useGetPendingSignMessageRequestsQuery,
  useSignMessageHardwareMutation,
} from '../../../common/slices/api.slice'
import {
  hardwareOperationMatchesSignMessage,
  hardwareOperationMatchesTransaction,
} from './hardware_wallet_retry'

// style
import {
  StyledWrapper,
  Title,
  IconWrapper,
} from './connect_hardware_wallet_panel.style'
import { Row, Column } from '../../shared/style'

import { getDeviceNameFromDevice } from '../../../common/async/hardware'

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

  const hardwareWalletOperation = useUnsafePanelSelector(
    PanelSelectors.hardwareWalletOperation,
  )

  // mutations
  const [signMessageHardware] = useSignMessageHardwareMutation()

  // queries
  const { data: signMessageData } = useGetPendingSignMessageRequestsQuery()
  const request = signMessageData?.at(0)

  const { account: signMessageAccount } = useAccountQuery(request?.accountId)
  const { account: operationAccount } = useAccountFromAddressQuery(
    hardwareWalletOperation?.accountUniqueKey,
  )

  const [deviceName, setDeviceName] = React.useState<string>('')

  // pending transactions
  const {
    onConfirm: onConfirmTransaction,
    selectedPendingTransaction,
    fromAccount: confirmTransactionAccount,
  } = usePendingTransactions()

  const account =
    operationAccount || signMessageAccount || confirmTransactionAccount

  // memos
  const isConnected = React.useMemo((): boolean => {
    return hardwareWalletCode !== 'deviceNotConnected'
  }, [hardwareWalletCode])

  const title = React.useMemo(() => {
    if (!account) {
      return ''
    }

    if (
      hardwareWalletCode === 'deviceBusy'
      || hardwareWalletCode === undefined
    ) {
      return getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_CONFIRMATION)
    }

    if (hardwareWalletCode === 'deviceNotConnected') {
      return getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_CONNECT).replace(
        '$1',
        account.name,
      )
    }

    const network = getAppName(account.accountId.coin)
    return getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_OPEN_APP)
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

  const updateDeviceName = React.useCallback(async () => {
    if (!account || !account.hardware) {
      return
    }
    const name = await getDeviceNameFromDevice(
      account.hardware.vendor,
      account.accountId.coin,
    )
    setDeviceName(name.success ? name.deviceName : '')
  }, [account])

  useInterval(updateDeviceName, isConnected ? 500 : null)

  // Retry only the operation that opened this panel. Other pending queues
  // stay pending until their own confirmation UI is shown.
  const retryHardwareOperation = React.useCallback(() => {
    if (
      hardwareOperationMatchesSignMessage(hardwareWalletOperation, request)
      && signMessageAccount
      && signMessageAccount.accountId.uniqueKey === request.accountId.uniqueKey
      && isHardwareAccount(signMessageAccount.accountId)
    ) {
      signMessageHardware({
        account: signMessageAccount,
        request,
      })
      return
    }

    if (
      hardwareOperationMatchesTransaction(
        hardwareWalletOperation,
        selectedPendingTransaction,
      )
    ) {
      onConfirmTransaction()
    }
  }, [
    hardwareWalletOperation,
    request,
    signMessageAccount,
    selectedPendingTransaction,
    signMessageHardware,
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
          <Title>{getLocale(S.BRAVE_WALLET_ADD_ACCOUNT_CONNECT)}</Title>
        </Row>
        <Label color={isConnected ? 'green' : 'red'}>
          <Icon
            slot='icon-before'
            name={isConnected ? 'check-circle-filled' : 'close-circle-filled'}
          />
          {isConnected
            ? getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_CONNECTED)
                .replace('$1', account.name)
                .replace('$2', deviceName)
            : getLocale(
                S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_DISCONNECTED,
              ).replace('$1', account.name)}
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
              {getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_PANEL_INSTRUCTIONS)}
            </Button>
          </Row>
        </Column>
      </Column>
      <Column
        width='100%'
        padding='16px'
        gap='16px'
      >
        <Row>
          <Button
            kind='outline'
            size='medium'
            onClick={onCancelConnect}
            isDisabled={hardwareWalletCode === 'deviceBusy'}
          >
            {getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}
          </Button>
        </Row>
      </Column>
    </StyledWrapper>
  )
}

export default ConnectHardwareWalletPanel
