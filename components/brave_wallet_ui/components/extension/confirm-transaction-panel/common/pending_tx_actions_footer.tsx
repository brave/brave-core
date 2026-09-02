// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Hooks
import {
  useUnsafeUISelector, //
} from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// Types
import { ParsedTransaction } from '../../../../utils/tx-utils'

// Utils
import { getLocale } from '../../../../../common/locale'

// components
import { TransactionWarnings, TransactionWarning } from './tx_warnings'

// Styled components
import { Row } from '../../../shared/style'
import { QueueStepButton } from './style'
import {
  FooterButtonRow,
  rejectAllButtonRowPadding,
  FooterContainer,
} from './pending_tx_actions_footer.style'

interface Props {
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
  isAccountSyncing?: boolean
  isShieldingFunds?: boolean
  isUnshieldingFunds?: boolean
  isMigratingFunds?: boolean
}

type Warning = TransactionWarning

export function PendingTransactionActionsFooter({
  isWarningCollapsed,
  setIsWarningCollapsed,
  isConfirmButtonDisabled,
  rejectAllTransactions,
  transactionDetails,
  transactionsQueueLength,
  insufficientFundsForGasError,
  insufficientFundsError,
  onReject,
  onConfirm,
  isAccountSyncing,
  isShieldingFunds,
  isUnshieldingFunds,
  isMigratingFunds,
}: Props) {
  // selectors
  const submittingTransaction = useUnsafeUISelector(
    UISelectors.submittingTransaction,
  )

  // state
  const [isWarningDismissed, setIsWarningDismissed] = React.useState(false)
  const [transactionConfirmed, setTranactionConfirmed] = React.useState(false)
  const [queueLength, setQueueLength] = React.useState<number | undefined>(
    undefined,
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
    return [
      transactionDetails?.contractAddressError,
      transactionDetails?.sameAddressError,
      transactionDetails?.missingGasLimitError,
      insufficientFundsForGasError
        ? getLocale(S.BRAVE_WALLET_SWAP_INSUFFICIENT_FUNDS_FOR_GAS)
        : undefined,
      !insufficientFundsForGasError && insufficientFundsError
        ? getLocale(S.BRAVE_WALLET_SWAP_INSUFFICIENT_BALANCE)
        : undefined,
    ]
      .filter((warning): warning is string => Boolean(warning))
      .map(
        (warning): Warning => ({
          message: warning,
          severity: 'warning',
        }),
      )
  }, [transactionDetails, insufficientFundsForGasError, insufficientFundsError])

  const hasWarnings = Boolean(warnings.length)

  const isTransactionConfirmedOrSubmitting =
    transactionConfirmed || !!submittingTransaction
  const isConfirmButtonDisabledOrSubmitting =
    isConfirmButtonDisabled || !!submittingTransaction

  const confirmButtonText = React.useMemo((): string => {
    if (isAccountSyncing) {
      return getLocale(S.BRAVE_WALLET_SYNCING)
    }
    if (isShieldingFunds) {
      return getLocale(S.BRAVE_WALLET_SHIELD_ZEC)
    }
    if (isUnshieldingFunds) {
      return getLocale(S.BRAVE_WALLET_UNSHIELD_ZEC)
    }
    if (isMigratingFunds) {
      return getLocale(S.BRAVE_WALLET_MIGRATE_ZEC)
    }
    return getLocale(S.BRAVE_WALLET_ALLOW_SPEND_CONFIRM_BUTTON)
  }, [isAccountSyncing, isShieldingFunds, isUnshieldingFunds, isMigratingFunds])

  const { confirmButton, rejectButton } = React.useMemo(() => {
    return {
      confirmButton: (
        <Button
          kind={hasWarnings ? 'outline' : 'filled'}
          onClick={onClickConfirmTransaction}
          disabled={isConfirmButtonDisabledOrSubmitting}
          isDisabled={isConfirmButtonDisabledOrSubmitting}
          isLoading={isTransactionConfirmedOrSubmitting}
        >
          {confirmButtonText}
        </Button>
      ),
      rejectButton: (
        <Button
          kind={hasWarnings ? 'filled' : 'outline'}
          onClick={onReject}
          disabled={isTransactionConfirmedOrSubmitting}
          isDisabled={isTransactionConfirmedOrSubmitting}
        >
          {getLocale(S.BRAVE_WALLET_ALLOW_SPEND_REJECT_BUTTON)}
        </Button>
      ),
    }
  }, [
    hasWarnings,
    onClickConfirmTransaction,
    isTransactionConfirmedOrSubmitting,
    isConfirmButtonDisabledOrSubmitting,
    onReject,
    confirmButtonText,
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
          classifyAs='issues'
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
            {getLocale(S.BRAVE_WALLET_QUEUE_REJECT_ALL).replace(
              '$1',
              transactionsQueueLength.toString(),
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
