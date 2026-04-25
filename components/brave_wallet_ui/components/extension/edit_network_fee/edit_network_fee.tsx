// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { MaxPriorityFeeTypes } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  parseTransactionFeesWithoutPrices, //
} from '../../../utils/tx-utils'
import Amount from '../../../utils/amount'

// Components
import {
  SuggestedMaxPriorityFeeSelector, //
} from './suggested_max_priority_fee_selector/suggested_max_priority_fee_selector'
import {
  CustomNetworkFee, //
} from './custom_network_fee/custom_network_fee'
import {
  BottomSheet, //
} from '../../shared/bottom_sheet/bottom_sheet'

// Hooks
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'

interface Props {
  isOpen: boolean
  onCancel: () => void
}

export function EditNetworkFee(props: Props) {
  const { isOpen, onCancel } = props

  // Hooks
  const {
    suggestedMaxPriorityFeeOptions,
    updateUnapprovedTransactionGasFields,
    baseFeePerGas,
    transactionsNetwork,
    selectedPendingTransaction,
  } = usePendingTransactions()

  const transactionFees = React.useMemo(
    () =>
      selectedPendingTransaction
        ? parseTransactionFeesWithoutPrices(selectedPendingTransaction)
        : null,
    [selectedPendingTransaction],
  )
  const { isEIP1559Transaction, gasLimit } = transactionFees || {}

  // State
  const [suggestedMaxPriorityFee, setSuggestedMaxPriorityFee] =
    React.useState<MaxPriorityFeeTypes>('average')
  const [suggestedOrCustom, setSuggestedOrCustom] = React.useState<
    'suggested' | 'custom'
  >(isEIP1559Transaction ? 'suggested' : 'custom')

  // Methods
  const onUpdatedSuggestedMaxPriorityFee = React.useCallback(
    (value: MaxPriorityFeeTypes) => {
      const maxPriorityFee = suggestedMaxPriorityFeeOptions.find(
        (option) => option.id === value,
      )?.fee

      if (!maxPriorityFee || !selectedPendingTransaction || !gasLimit) {
        return
      }
      setSuggestedMaxPriorityFee(value)
      updateUnapprovedTransactionGasFields({
        chainId: selectedPendingTransaction.chainId,
        txMetaId: selectedPendingTransaction.id,
        gasLimit: new Amount(gasLimit).toHex(),
        maxPriorityFeePerGas: maxPriorityFee,
        maxFeePerGas: new Amount(baseFeePerGas).plus(maxPriorityFee).toHex(),
      })
      onCancel()
    },
    [
      selectedPendingTransaction,
      baseFeePerGas,
      updateUnapprovedTransactionGasFields,
      suggestedMaxPriorityFeeOptions,
      gasLimit,
      onCancel,
    ],
  )

  if (!selectedPendingTransaction || !transactionsNetwork) {
    return null
  }

  return (
    <BottomSheet
      isOpen={isOpen}
      title={
        suggestedOrCustom === 'suggested'
          ? getLocale('braveWalletNetworkFee')
          : getLocale('braveWalletCustomFeeAmount')
      }
      onClose={onCancel}
    >
      {suggestedOrCustom === 'suggested' ? (
        <SuggestedMaxPriorityFeeSelector
          transactionInfo={selectedPendingTransaction}
          selectedNetwork={transactionsNetwork}
          baseFeePerGas={baseFeePerGas}
          suggestedMaxPriorityFee={suggestedMaxPriorityFee}
          suggestedMaxPriorityFeeOptions={suggestedMaxPriorityFeeOptions}
          onClickCustom={() => setSuggestedOrCustom('custom')}
          setSuggestedMaxPriorityFee={onUpdatedSuggestedMaxPriorityFee}
        />
      ) : (
        <CustomNetworkFee
          transactionInfo={selectedPendingTransaction}
          selectedNetwork={transactionsNetwork}
          baseFeePerGas={baseFeePerGas}
          onClose={onCancel}
          onBack={() => setSuggestedOrCustom('suggested')}
          onUpdateCustomNetworkFee={updateUnapprovedTransactionGasFields}
        />
      )}
    </BottomSheet>
  )
}
