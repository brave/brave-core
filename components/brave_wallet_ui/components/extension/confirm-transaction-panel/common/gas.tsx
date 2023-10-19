// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// hooks
import { usePendingTransactions } from '../../../../common/hooks/use-pending-transaction'

// components
import EditGas, { MaxPriorityPanels } from '../../edit-gas/edit-gas'

interface Props {
  onCancel: () => void
}

export function EditPendingTransactionGas(props: Props) {
  const { onCancel } = props

  const [suggestedSliderStep, setSuggestedSliderStep] =
    React.useState<string>('1')
  const [maxPriorityPanel, setMaxPriorityPanel] =
    React.useState<MaxPriorityPanels>(MaxPriorityPanels.setSuggested)

  const {
    suggestedMaxPriorityFeeChoices,
    updateUnapprovedTransactionGasFields,
    baseFeePerGas,
    transactionsNetwork,
    selectedPendingTransaction
  } = usePendingTransactions()

  if (!selectedPendingTransaction || !transactionsNetwork) {
    return null
  }

  return (
    <EditGas
      transactionInfo={selectedPendingTransaction}
      onCancel={onCancel}
      selectedNetwork={transactionsNetwork}
      baseFeePerGas={baseFeePerGas}
      suggestedMaxPriorityFeeChoices={suggestedMaxPriorityFeeChoices}
      updateUnapprovedTransactionGasFields={
        updateUnapprovedTransactionGasFields
      }
      suggestedSliderStep={suggestedSliderStep}
      setSuggestedSliderStep={setSuggestedSliderStep}
      maxPriorityPanel={maxPriorityPanel}
      setMaxPriorityPanel={setMaxPriorityPanel}
    />
  )
}
