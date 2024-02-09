// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Hooks
import {
  useSignSolanaTransactionsQueue //
} from '../../../common/hooks/use_sign_solana_tx_queue'

// Components
import SignTransactionPanel from '../sign-panel/sign-transaction-panel'
import { LoadingPanel } from '../loading_panel/loading_panel'

interface Props {
  signMode: 'signTx' | 'signAllTxs'
}

export const PendingSignatureRequestsPanel: React.FC<Props> = ({
  signMode
}) => {
  // custom hooks
  const {
    isDisabled,
    network,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedQueueData,
    signingAccount
  } = useSignSolanaTransactionsQueue(signMode)

  // render

  // Loading/Fetching network
  if (!network) {
    return <LoadingPanel />
  }

  // Default (not simulated)
  return (
    <SignTransactionPanel
      signMode={signMode}
      isSigningDisabled={isDisabled}
      network={network}
      queueLength={queueLength}
      queueNextSignTransaction={queueNextSignTransaction}
      queueNumber={queueNumber}
      selectedQueueData={selectedQueueData}
      signingAccount={signingAccount}
    />
  )
}

export default PendingSignatureRequestsPanel
