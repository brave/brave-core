// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'
import { ParsedTransaction } from '../../../../utils/tx-utils'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  translateSimulationWarning //
} from '../../../../utils/tx-simulation-utils'

// components
import { TransactionWarnings } from './tx_warnings'

// Styled components
import { LeoSquaredButton, Row } from '../../../shared/style'
import { QueueStepButton } from './style'
import {
  FooterButtonRow,
  rejectAllButtonRowPadding,
  FooterContainer
} from './pending_tx_actions_footer.style'

interface Props {
  blowfishWarnings?: BraveWallet.BlowfishWarning[]
  setIsWarningCollapsed?: React.Dispatch<React.SetStateAction<boolean>>
  isWarningCollapsed?: boolean
  isConfirmButtonDisabled: boolean
  rejectAllTransactions?: (() => Promise<void>) | (() => void)
  transactionDetails: ParsedTransaction | undefined
  transactionsQueueLength: number
  /** omit this prop if you don't want to display gas errors */
  insufficientFundsForGasError?: boolean
  /** omit this prop if you don't want to display the error */
  insufficientFundsError?: boolean
  onReject: () => void
  onConfirm: (() => Promise<void>) | (() => void)
}

type Warning = Pick<BraveWallet.BlowfishWarning, 'message' | 'severity'>

export function PendingTransactionActionsFooter({
  isWarningCollapsed,
  setIsWarningCollapsed,
  blowfishWarnings,
  isConfirmButtonDisabled,
  rejectAllTransactions,
  transactionDetails,
  transactionsQueueLength,
  insufficientFundsForGasError,
  insufficientFundsError,
  onReject,
  onConfirm
}: Props) {
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
      return blowfishWarnings.map((w) => ({
        message: translateSimulationWarning(w),
        severity: w.severity
      }))
    }

    return [
      transactionDetails?.contractAddressError,
      transactionDetails?.sameAddressError,
      transactionDetails?.missingGasLimitError,
      insufficientFundsForGasError
        ? getLocale('braveWalletSwapInsufficientFundsForGas')
        : undefined,
      !insufficientFundsForGasError && insufficientFundsError
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
    insufficientFundsForGasError,
    insufficientFundsError
  ])

  const hasWarnings = Boolean(warnings.length)

  const { confirmButton, rejectButton } = React.useMemo(() => {
    return {
      confirmButton: (
        <LeoSquaredButton
          kind={hasWarnings ? 'outline' : 'filled'}
          onClick={onClickConfirmTransaction}
          disabled={isConfirmButtonDisabled}
          isDisabled={isConfirmButtonDisabled}
          isLoading={transactionConfirmed}
        >
          {getLocale('braveWalletAllowSpendConfirmButton')}
        </LeoSquaredButton>
      ),
      rejectButton: (
        <LeoSquaredButton
          kind={hasWarnings ? 'filled' : 'outline'}
          onClick={onReject}
          disabled={transactionConfirmed}
          isDisabled={transactionConfirmed}
        >
          {getLocale('braveWalletAllowSpendRejectButton')}
        </LeoSquaredButton>
      )
    }
  }, [
    hasWarnings,
    onClickConfirmTransaction,
    isConfirmButtonDisabled,
    transactionConfirmed,
    onReject
  ])

  // computed
  const displayIssuesAsRisks = Boolean(blowfishWarnings?.length)

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
          classifyAs={displayIssuesAsRisks ? 'risks' : 'issues'}
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

      {rejectAllTransactions && transactionsQueueLength > 1 && (
        <Row padding={rejectAllButtonRowPadding}>
          <QueueStepButton onClick={rejectAllTransactions}>
            {getLocale('braveWalletQueueRejectAll').replace(
              '$1',
              transactionsQueueLength.toString()
            )}
          </QueueStepButton>
        </Row>
      )}

      <FooterButtonRow>
        {hasWarnings ? (
          <>
            <div>{confirmButton}</div>
            <div>{rejectButton}</div>
          </>
        ) : (
          <>
            <div>{rejectButton}</div>
            <div>{confirmButton}</div>
          </>
        )}
      </FooterButtonRow>
    </FooterContainer>
  )
}
