// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Utils
import { getLocale } from '$web-common/locale'

// Hooks
import { usePendingTransactions } from '../../../../common/hooks/use-pending-transaction'

// Components
import { NavButton, Panel } from '../..'
import { SuccessIcon, Title } from './complete.style'
import {
  ButtonRow,
  PendingTransactionsRow,
  TransactionStatusDescription
} from '../common/common.style'

interface Props {
  headerTitle: string
  description: string
  isPrimaryCTADisabled: boolean
  primaryCTAText: string
  onClose: () => void
  onClickSecondaryCTA: () => void
  onClickPrimaryCTA: () => void
}

export const TransactionComplete = (props: Props) => {
  const {
    headerTitle,
    description,
    isPrimaryCTADisabled,
    primaryCTAText,
    onClose,
    onClickPrimaryCTA,
    onClickSecondaryCTA
  } = props

  const { transactionsQueueLength } = usePendingTransactions()

  return (
    <Panel navAction={onClose} title={headerTitle} headerStyle='slim'>
      <SuccessIcon />
      <Title>{getLocale('braveWalletTransactionCompleteTitle')}</Title>
      <TransactionStatusDescription>{description}</TransactionStatusDescription>

      {transactionsQueueLength >= 1 && (
        <PendingTransactionsRow>
          {transactionsQueueLength} more transactions pending.
        </PendingTransactionsRow>
      )}

      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletTransactionCompleteReceiptCTA')}
          onSubmit={onClickSecondaryCTA}
        />
        <NavButton
          buttonType='primary'
          text={primaryCTAText}
          onSubmit={onClickPrimaryCTA}
          disabled={isPrimaryCTADisabled}
        />
      </ButtonRow>
    </Panel>
  )
}
