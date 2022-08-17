// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import EditGas, { MaxPriorityPanels } from '../../edit-gas'
import { usePendingTransactions } from '../../../../common/hooks/use-pending-transaction'
import { useSelector } from 'react-redux'
import { WalletState } from '../../../../constants/types'

interface Props {
  onCancel: () => void
}

export function EditPendingTransactionGas (props: Props) {
  const { onCancel } = props

  // redux
  const { selectedPendingTransaction: transactionInfo } = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet
  )

  const [suggestedSliderStep, setSuggestedSliderStep] = React.useState<string>('1')
  const [maxPriorityPanel, setMaxPriorityPanel] = React.useState<MaxPriorityPanels>(
    MaxPriorityPanels.setSuggested
  )

  const {
    suggestedMaxPriorityFeeChoices,
    updateUnapprovedTransactionGasFields,
    baseFeePerGas,
    findAssetPrice,
    transactionsNetwork
  } = usePendingTransactions()

  if (!transactionInfo) {
    return null
  }

  return (
    <EditGas
      transactionInfo={transactionInfo}
      onCancel={onCancel}
      networkSpotPrice={findAssetPrice(transactionsNetwork.symbol)}
      selectedNetwork={transactionsNetwork}
      baseFeePerGas={baseFeePerGas}
      suggestedMaxPriorityFeeChoices={suggestedMaxPriorityFeeChoices}
      updateUnapprovedTransactionGasFields={updateUnapprovedTransactionGasFields}
      suggestedSliderStep={suggestedSliderStep}
      setSuggestedSliderStep={setSuggestedSliderStep}
      maxPriorityPanel={maxPriorityPanel}
      setMaxPriorityPanel={setMaxPriorityPanel}
    />
  )
}
