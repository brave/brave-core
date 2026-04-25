// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Hooks
import {
  useSignCardanoTransactionsQueue, //
} from '../../../common/hooks/use_sign_cardano_tx_queue'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

// Components
import SignCardanoTxPanel from '../sign-panel/sign_cardano_tx_panel'
import { LoadingPanel } from '../loading_panel/loading_panel'

// Style
import { LongWrapper } from '../../../stories/style'

export const PendingSignCardanoTransactionRequestsPanel: React.FC = () => {
  // custom hooks
  const {
    isDisabled,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedRequest,
    signingAccount,
  } = useSignCardanoTransactionsQueue()

  const { data: network } = useGetNetworkQuery(
    selectedRequest ? selectedRequest.chainId : skipToken,
  )

  // Loading
  if (!network || !selectedRequest || !signingAccount) {
    return <LoadingPanel />
  }

  // Default (not simulated)
  return (
    <LongWrapper padding='0px'>
      <SignCardanoTxPanel
        isSigningDisabled={isDisabled}
        network={network}
        queueLength={queueLength}
        queueNextSignTransaction={queueNextSignTransaction}
        queueNumber={queueNumber}
        selectedRequest={selectedRequest}
        signingAccount={signingAccount}
      />
    </LongWrapper>
  )
}

export default PendingSignCardanoTransactionRequestsPanel
