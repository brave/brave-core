// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Hooks
import { useUnsafePanelSelector } from '../../../common/hooks/use-safe-selector'
import { PanelSelectors } from '../../../panel/selectors'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { Row } from '../../shared/style'

export interface Props {
  onConfirm: (() => Promise<void>) | (() => void)
  onReject: () => void
  isConfirmButtonDisabled: boolean
  isAccountSyncing?: boolean
  isShieldingFunds?: boolean
  isUnshieldingFunds?: boolean
}

export const ConfirmRejectButtons = (props: Props) => {
  const {
    onConfirm,
    onReject,
    isConfirmButtonDisabled,
    isAccountSyncing,
    isShieldingFunds,
    isUnshieldingFunds,
  } = props

  // selectors
  const submittingTransaction = useUnsafePanelSelector(
    PanelSelectors.submittingTransaction,
  )

  // State
  const [transactionConfirmed, setTranactionConfirmed] = React.useState(false)

  // Methods
  const onClickConfirmTransaction = React.useCallback(async () => {
    // Sets transactionConfirmed state to disable the send button to prevent
    // being clicked again and submitting the same transaction.
    setTranactionConfirmed(true)
    await onConfirm()
  }, [onConfirm])

  const isTransactionConfirmedOrSubmitting =
    transactionConfirmed || !!submittingTransaction
  const isConfirmButtonDisabledOrSubmitting =
    isConfirmButtonDisabled || !!submittingTransaction

  return (
    <Row
      padding='16px'
      gap='8px'
    >
      <Button
        kind='outline'
        size='medium'
        onClick={onReject}
        disabled={isTransactionConfirmedOrSubmitting}
        isDisabled={isTransactionConfirmedOrSubmitting}
      >
        {getLocale('braveWalletAllowSpendRejectButton')}
      </Button>
      <Button
        kind='filled'
        size='medium'
        onClick={onClickConfirmTransaction}
        disabled={isConfirmButtonDisabledOrSubmitting}
        isDisabled={isConfirmButtonDisabledOrSubmitting}
        isLoading={isTransactionConfirmedOrSubmitting}
      >
        {isAccountSyncing
          ? getLocale('braveWalletSyncing')
          : isShieldingFunds
            ? getLocale('braveWalletShieldZEC')
            : isUnshieldingFunds
              ? getLocale('braveWalletUnshieldZEC')
              : getLocale('braveWalletAllowSpendConfirmButton')}
      </Button>
    </Row>
  )
}
