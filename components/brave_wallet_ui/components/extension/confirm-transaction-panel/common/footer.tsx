// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'

// Styled components
import {
  ButtonRow,
  ConfirmingButton,
  ConfirmingButtonText,
  ErrorText,
  FooterContainer,
  LoadIcon,
  QueueStepButton
} from './style'
import { NavButton } from '../../buttons'

// Hooks
import { usePendingTransactions } from '../../../../common/hooks/use-pending-transaction'

interface Props {
  onConfirm: () => void
  onReject: () => void
  rejectButtonType?: 'reject' | 'cancel'
}

export function Footer (props: Props) {
  const { onReject, onConfirm, rejectButtonType } = props

  const {
    isConfirmButtonDisabled,
    rejectAllTransactions,
    transactionDetails,
    transactionsQueueLength
  } = usePendingTransactions()

  const [transactionConfirmed, setTranactionConfirmed] = React.useState<boolean>(false)
  const [queueLength, setQueueLength] = React.useState<number | undefined>(undefined)

  React.useEffect(() => {
    // This will update the transactionConfirmed state back to false
    // if there are more than 1 transactions in the queue.
    if (queueLength !== transactionsQueueLength || queueLength === undefined) {
      setTranactionConfirmed(false)
    }
  }, [queueLength, transactionsQueueLength])

  const onClickConfirmTransaction = React.useCallback(() => {
    // Checks to see if there are multiple transactions in the queue,
    // if there is we keep track of the length of the last confirmed transaction.
    if (transactionsQueueLength > 1) {
      setQueueLength(transactionsQueueLength)
    }
    // Sets transactionConfirmed state to disable the send button to prevent
    // being clicked again and submitting the same transaction.
    setTranactionConfirmed(true)
    onConfirm()
  }, [transactionsQueueLength, onConfirm])

  return (
    <FooterContainer>
      {transactionsQueueLength > 1 && (
        <QueueStepButton needsMargin={false} onClick={rejectAllTransactions}>
          {getLocale('braveWalletQueueRejectAll').replace('$1', transactionsQueueLength.toString())}
        </QueueStepButton>
      )}

      {transactionDetails &&
        [
          transactionDetails.contractAddressError,
          transactionDetails.sameAddressError,
          transactionDetails.missingGasLimitError
        ].map((error, index) => <ErrorText key={`${index}-${error}`}>{error}</ErrorText>)}

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
          />
        )}
      </ButtonRow>
    </FooterContainer>
  )
}
