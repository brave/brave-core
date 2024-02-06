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

// components
import { NavButton } from '../../buttons'
import { TransactionWarnings } from './tx_warnings'

// Styled components
import { Row } from '../../../shared/style'
import {
  ConfirmingButton,
  ConfirmingButtonText,
  FooterContainer,
  LoadIcon,
  QueueStepButton
} from './style'
import {
  FooterButtonRow,
  rejectAllButtonRowPadding
} from './pending_tx_actions_footer.style'

interface Props {
  rejectButtonType?: 'reject' | 'cancel'
  blowfishWarnings?: BraveWallet.BlowfishWarning[]
  setIsWarningCollapsed?: React.Dispatch<React.SetStateAction<boolean>>
  isWarningCollapsed?: boolean
  isConfirmButtonDisabled: boolean
  rejectAllTransactions: (() => Promise<void>) | (() => void)
  transactionDetails: ParsedTransaction | undefined
  transactionsQueueLength: number
  /** omit this prop if you don't want to display gas errors */
  insufficientFundsForGasError?: boolean
  /** omit this prop if you don't want to display the error */
  insufficientFundsError?: boolean
  onReject: () => void
  onConfirm: () => Promise<void>
}

type Warning = Pick<BraveWallet.BlowfishWarning, 'message' | 'severity'>

export function PendingTransactionActionsFooter({
  isWarningCollapsed,
  setIsWarningCollapsed,
  blowfishWarnings,
  rejectButtonType,
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
      return blowfishWarnings
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

      {transactionsQueueLength > 1 && (
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
      </FooterButtonRow>
    </FooterContainer>
  )
}
