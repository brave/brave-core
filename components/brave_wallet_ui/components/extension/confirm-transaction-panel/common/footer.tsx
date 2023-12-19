// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  translateSimulationWarning //
} from '../../../../utils/tx-simulation-utils'

// components
import { NavButton } from '../../buttons'
import { TransactionWarnings } from './tx_warnings'

// Styled components
import { Row } from '../../../shared/style'
import {
  FooterButtonRow,
  ConfirmingButton,
  ConfirmingButtonText,
  FooterContainer,
  LoadIcon,
  QueueStepButton,
  queueStepButtonRowPadding
} from './style'

// Hooks
import { usePendingTransactions } from '../../../../common/hooks/use-pending-transaction'

interface Props {
  rejectButtonType?: 'reject' | 'cancel'
  showGasErrors?: boolean
  disableConfirmation?: boolean
  blowfishWarnings?: BraveWallet.BlowfishWarning[]
  setIsWarningCollapsed?: React.Dispatch<React.SetStateAction<boolean>>
  isWarningCollapsed?: boolean
}

type Warning = Pick<BraveWallet.BlowfishWarning, 'message' | 'severity'>

export function Footer({
  isWarningCollapsed,
  setIsWarningCollapsed,
  blowfishWarnings,
  rejectButtonType,
  showGasErrors,
  disableConfirmation
}: Props) {
  // custom hooks
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

  // state
  const [isWarningDismissed, setIsWarningDismissed] = React.useState(false)
  const [transactionConfirmed, setTranactionConfirmed] = React.useState(false)
  const [queueLength, setQueueLength] = React.useState<number | undefined>(
    undefined
  )

  // methods
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

  // memos
  const warnings: Warning[] = React.useMemo(() => {
    if (blowfishWarnings?.length) {
      return blowfishWarnings.map(
        (warning): Warning => ({
          message: translateSimulationWarning(warning),
          severity: warning.severity
        })
      )
    }

    return [
      transactionDetails?.contractAddressError,
      transactionDetails?.sameAddressError,
      transactionDetails?.missingGasLimitError,
      showGasErrors && insufficientFundsForGasError
        ? getLocale('braveWalletSwapInsufficientFundsForGas')
        : undefined,
      showGasErrors && !insufficientFundsForGasError && insufficientFundsError
        ? getLocale('braveWalletSwapInsufficientBalance')
        : undefined
    ]
      .filter((warning): warning is string => Boolean(warning))
      .map(
        (warning): Warning => ({
          message: warning,
          severity: BraveWallet.BlowfishWarningSeverity.kWarning
        })
      )
  }, [
    transactionDetails,
    blowfishWarnings,
    showGasErrors,
    insufficientFundsForGasError,
    insufficientFundsError
  ])

  // effects
  React.useEffect(() => {
    // This will update the transactionConfirmed state back to false
    // if there are more than 1 transactions in the queue.
    if (queueLength !== transactionsQueueLength || queueLength === undefined) {
      setTranactionConfirmed(false)
    }
  }, [queueLength, transactionsQueueLength])

  // render
  return (
    <FooterContainer>
      {!isWarningDismissed && (
        <TransactionWarnings
          warnings={warnings}
          isWarningCollapsed={isWarningCollapsed ?? true}
          setIsWarningCollapsed={setIsWarningCollapsed}
          onDismiss={
            setIsWarningDismissed
              ? () => {
                  setIsWarningDismissed(true)
                  setIsWarningCollapsed?.(true)
                }
              : undefined
          }
        />
      )}

      {transactionsQueueLength > 1 && (
        <Row padding={queueStepButtonRowPadding}>
          <QueueStepButton onClick={rejectAllTransactions}>
            {getLocale('braveWalletQueueRejectAll').replace(
              '$1',
              transactionsQueueLength.toString()
            )}
          </QueueStepButton>
        </Row>
      )}

      <FooterButtonRow>
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
            disabled={disableConfirmation || isConfirmButtonDisabled}
            minWidth='45%'
          />
        )}
      </FooterButtonRow>
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
      <FooterButtonRow>
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
      </FooterButtonRow>
    </FooterContainer>
  )
}
