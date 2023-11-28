// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'

// components
import { NavButton } from '../../buttons'
import { TxWarningBanner } from './tx_warning_banner'

// Styled components
import { TransactionText } from '../style'
import {
  ButtonRow,
  ConfirmingButton,
  ConfirmingButtonText,
  ErrorText,
  FooterContainer,
  LoadIcon,
  NetworkFeeRow,
  QueueStepButton
} from './style'

// Hooks
import { usePendingTransactions } from '../../../../common/hooks/use-pending-transaction'

interface Props {
  rejectButtonType?: 'reject' | 'cancel'
  showGasErrors?: boolean
}

export function Footer(props: Props) {
  const { rejectButtonType, showGasErrors } = props

  const {
    isConfirmButtonDisabled,
    rejectAllTransactions,
    transactionDetails,
    transactionsQueueLength,
    insufficientFundsForGasError,
    insufficientFundsError,
    onReject,
    onConfirm
  } = usePendingTransactions()

  const [transactionConfirmed, setTranactionConfirmed] =
    React.useState<boolean>(false)
  const [queueLength, setQueueLength] = React.useState<number | undefined>(
    undefined
  )

  React.useEffect(() => {
    // This will update the transactionConfirmed state back to false
    // if there are more than 1 transactions in the queue.
    if (queueLength !== transactionsQueueLength || queueLength === undefined) {
      setTranactionConfirmed(false)
    }
  }, [queueLength, transactionsQueueLength])

  const onClickConfirmTransaction = React.useCallback(async () => {
    // Checks to see if there are multiple transactions in the queue, if there
    // is we keep track of the length of the last confirmed transaction.
    if (transactionsQueueLength > 1) {
      setQueueLength(transactionsQueueLength)
    }
    // Sets transactionConfirmed state to disable the send button to prevent
    // being clicked again and submitting the same transaction.
    setTranactionConfirmed(true)
    await onConfirm()
  }, [transactionsQueueLength, onConfirm])

  return (
    <FooterContainer>
      {transactionsQueueLength > 1 && (
        <QueueStepButton
          needsMargin={false}
          onClick={rejectAllTransactions}
        >
          {getLocale('braveWalletQueueRejectAll').replace(
            '$1',
            transactionsQueueLength.toString()
          )}
        </QueueStepButton>
      )}

      {transactionDetails &&
        [
          transactionDetails.contractAddressError,
          transactionDetails.sameAddressError,
          transactionDetails.missingGasLimitError
        ]
          .filter(Boolean)
          .map((error) => <ErrorText key={error}>{error}</ErrorText>)}

      {showGasErrors && insufficientFundsForGasError && (
        <NetworkFeeRow>
          <TransactionText hasError={true}>
            {getLocale('braveWalletSwapInsufficientFundsForGas')}
          </TransactionText>
        </NetworkFeeRow>
      )}

      {showGasErrors &&
        !insufficientFundsForGasError &&
        insufficientFundsError && (
          <TxWarningBanner messageLocale='braveWalletSwapInsufficientBalance' />
        )}

      <ButtonRow>
        <NavButton
          buttonType={rejectButtonType || 'reject'}
          text={
            rejectButtonType === 'cancel'
              ? getLocale('braveWalletButtonCancel')
              : getLocale('braveWalletAllowSpendRejectButton')
          }
          onSubmit={onReject}
          disabled={transactionConfirmed}
          minWidth='45%'
        />
        {transactionConfirmed ? (
          <ConfirmingButton>
            <ConfirmingButtonText>
              {getLocale('braveWalletAllowSpendConfirmButton')}
            </ConfirmingButtonText>
            <LoadIcon />
          </ConfirmingButton>
        ) : (
          <NavButton
            buttonType='confirm'
            text={getLocale('braveWalletAllowSpendConfirmButton')}
            onSubmit={onClickConfirmTransaction}
            disabled={isConfirmButtonDisabled}
            minWidth='45%'
          />
        )}
      </ButtonRow>
    </FooterContainer>
  )
}

export function SignTransactionFooter({
  onReject,
  onConfirm
}: {
  onConfirm: () => void | Promise<void>
  onReject: () => void
}) {
  // state
  const [isConfirming, setIsConfirming] = React.useState(false)

  // methods
  const onSign = async () => {
    setIsConfirming(true)
    await onConfirm()
  }

  // render
  return (
    <FooterContainer>
      <ButtonRow>
        <NavButton
          buttonType={'reject'}
          text={getLocale('braveWalletAllowSpendRejectButton')}
          onSubmit={onReject}
          disabled={isConfirming}
        />
        {isConfirming ? (
          <ConfirmingButton>
            <ConfirmingButtonText>
              {getLocale('braveWalletAllowSpendConfirmButton')}
            </ConfirmingButtonText>
            <LoadIcon />
          </ConfirmingButton>
        ) : (
          <NavButton
            buttonType='confirm'
            text={getLocale('braveWalletAllowSpendConfirmButton')}
            onSubmit={onSign}
            disabled={isConfirming}
          />
        )}
      </ButtonRow>
    </FooterContainer>
  )
}
