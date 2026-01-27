// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import LeoButton from '@brave/leo/react/button'
import { useDispatch } from 'react-redux'

// Actions
import * as WalletPanelActions from '../../../../panel/actions/wallet_panel_actions'

// Hooks
import {
  useRetryTransactionMutation, //
} from '../../../../common/slices/api.slice'

// Selectors
import {
  useUnsafeUISelector, //
} from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo, //
} from '../../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import {
  getIsSolanaAssociatedTokenAccountCreation,
  isBridgeTransaction,
  isSwapTransaction,
} from '../../../../utils/tx-utils'
import { getCoinFromTxDataUnion } from '../../../../utils/network-utils'

// Components
import {
  PostConfirmationHeader, //
} from '../common/post_confirmation_header'
import { TransactionIntent } from '../common/transaction_intent'

// Styled Components
import {
  // TransactionStatusDescription,
  Title,
  Wrapper,
  ErrorOrSuccessIconWrapper,
  ErrorOrSuccessIcon,
} from '../common/common.style'
import { Column, Row, Text } from '../../../shared/style'

interface Props {
  transaction: SerializableTransactionInfo
  onClose: () => void
  swapStatus?: BraveWallet.Gate3SwapStatus
}

export const TransactionFailedOrCanceled = (props: Props) => {
  const { transaction, onClose, swapStatus } = props

  // redux
  const transactionProviderErrorRegistry = useUnsafeUISelector(
    UISelectors.transactionProviderErrorRegistry,
  )

  // Hooks
  const [retryTx] = useRetryTransactionMutation()
  const dispatch = useDispatch()

  // Computed
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)
  const isBridge = isBridgeTransaction(transaction)
  const isSwap = isSwapTransaction(transaction)
  const isSwapOrBridge = isBridge || isSwap
  const isSolanaATACreation =
    getIsSolanaAssociatedTokenAccountCreation(transaction)

  // Check if swap was refunded
  const isSwapRefunded =
    swapStatus?.status === BraveWallet.Gate3SwapStatusCode.kRefunded

  const providerError = transactionProviderErrorRegistry[transaction.id]
  const errorCode =
    providerError?.code.providerError
    ?? providerError?.code.zcashProviderError
    ?? providerError?.code.bitcoinProviderError
    ?? providerError?.code.solanaProviderError
    ?? providerError?.code.filecoinProviderError
    ?? BraveWallet.ProviderError.kUnknown

  const errorDetails = providerError && `${errorCode}: ${providerError.message}`

  // Memos
  const sendSwapOrBridgeLocale = React.useMemo(() => {
    if (isBridge) {
      return 'braveWalletBridge'
    }
    if (isSwap) {
      return 'braveWalletSwap'
    }
    return 'braveWalletSend'
  }, [isBridge, isSwap])

  // Title for swap failures/refunds
  const failureTitle = React.useMemo(() => {
    if (isSolanaATACreation) {
      return getLocale('braveWalletTransactionFailedTitle')
    }
    if (isSwapRefunded) {
      return getLocale('braveWalletSwapRefunded')
    }
    return getLocale('braveWalletUnableToSendSwapOrBridge').replace(
      '$1',
      getLocale(sendSwapOrBridgeLocale).toLocaleLowerCase(),
    )
  }, [isSolanaATACreation, isSwapRefunded, sendSwapOrBridgeLocale])

  // Methods
  const onClickRetryTransaction = () => {
    retryTx({
      coinType: txCoinType,
      chainId: transaction.chainId,
      transactionId: transaction.id,
    })
    dispatch(WalletPanelActions.setSelectedTransactionId(undefined))
  }

  return (
    <Wrapper
      fullWidth={true}
      fullHeight={true}
      alignItems='center'
      justifyContent='space-between'
    >
      <PostConfirmationHeader
        onClose={onClose}
        showHelp={true}
      />
      <Column
        fullWidth={true}
        padding='0px 24px'
      >
        <ErrorOrSuccessIconWrapper kind='error'>
          <ErrorOrSuccessIcon
            kind='error'
            name='close'
          />
        </ErrorOrSuccessIconWrapper>
        <Title>{failureTitle}</Title>
        <TransactionIntent
          transaction={transaction}
          swapStatus={swapStatus}
        />
        {isSwapOrBridge && swapStatus?.internalStatus && (
          <Text
            textSize='14px'
            textColor='secondary'
            isBold={true}
          >
            {swapStatus.internalStatus}
          </Text>
        )}
        <Text
          textSize='12px'
          isBold={false}
          textColor='secondary'
        >
          {errorDetails}
        </Text>
      </Column>
      <Row
        padding='16px'
        gap='8px'
      >
        <LeoButton
          kind='outline'
          onClick={onClose}
        >
          {getLocale('braveWalletButtonClose')}
        </LeoButton>
        <LeoButton onClick={onClickRetryTransaction}>
          {getLocale('braveWalletButtonRetry')}
        </LeoButton>
      </Row>
    </Wrapper>
  )
}
