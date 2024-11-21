// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import LeoButton from '@brave/leo/react/button'

// Constants
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import {
  getCoinFromTxDataUnion //
} from '../../../../utils/network-utils'

// Components
import {
  PostConfirmationHeader //
} from '../common/post_confirmation_header'
import { SpeedUpAlert } from '../common/speed_up_alert'
import { TransactionIntent } from '../common/transaction_intent'

// Styled components
import { LoadingRing, StatusIcon, Wrapper, Title } from '../common/common.style'
import { Column, Row, Text, VerticalSpace } from '../../../shared/style'
import {
  isBridgeTransaction,
  isSwapTransaction
} from '../../../../utils/tx-utils'

interface Props {
  transaction: SerializableTransactionInfo
  onClose: () => void
  onShowCancelTransaction: () => void
  onClickViewInActivity: () => void
}

export const TransactionSubmittedOrSigned = (props: Props) => {
  const {
    transaction,
    onClose,
    onShowCancelTransaction,
    onClickViewInActivity
  } = props

  // State
  const [showSpeedUpAlert, setShowSpeedupAlert] = React.useState<boolean>(false)

  // Computed
  const isBridge = isBridgeTransaction(transaction)
  const isSwap = isSwapTransaction(transaction)
  const isERC20Approval =
    transaction.txType === BraveWallet.TransactionType.ERC20Approve
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)

  // Memos
  const statusIconName = React.useMemo(() => {
    if (isERC20Approval) {
      return 'lock-open'
    }
    if (isBridge) {
      return 'web3-bridge'
    }
    if (isSwap) {
      return 'swap-horizontal'
    }
    return 'send-filled'
  }, [isERC20Approval, isBridge, isSwap])

  React.useEffect(() => {
    const timeId = setTimeout(() => {
      setShowSpeedupAlert(true)
    }, 5000)
    return () => {
      clearTimeout(timeId)
    }
  }, [])

  return (
    <Wrapper
      fullWidth={true}
      fullHeight={true}
      alignItems='center'
      justifyContent='space-between'
    >
      <Column fullWidth={true}>
        <PostConfirmationHeader onClose={onClose} />
        <Column
          fullWidth={true}
          padding='0px 24px'
        >
          {txCoinType === BraveWallet.CoinType.ETH && showSpeedUpAlert ? (
            <SpeedUpAlert />
          ) : (
            <VerticalSpace space='114px' />
          )}
          <LoadingRing>
            <StatusIcon name={statusIconName} />
          </LoadingRing>
          <Title>
            {transaction.txStatus === BraveWallet.TransactionStatus.Submitted
              ? getLocale('braveWalletTransactionSubmittedTitle')
              : getLocale('braveWalletTransactionSignedTitle')}
          </Title>
          <TransactionIntent transaction={transaction} />
        </Column>
      </Column>
      <Column
        fullWidth={true}
        padding='16px'
        gap='16px'
      >
        <Text
          textSize='12px'
          textColor='tertiary'
          isBold={false}
        >
          {getLocale('braveWalletSafelyDismissWindow')}
        </Text>
        <Row gap='8px'>
          <LeoButton
            kind='plain'
            onClick={onShowCancelTransaction}
          >
            {getLocale('braveWalletTransactionCancel')}
          </LeoButton>
          <LeoButton onClick={onClickViewInActivity}>
            {getLocale('braveWalletViewInActivity')}
          </LeoButton>
        </Row>
      </Column>
    </Wrapper>
  )
}
