// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Hooks
import {
  useSignPolkadotTransactionsQueue, //
} from '../../../common/hooks/use_sign_polkadot_tx_queue'
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

// Components
import SignPolkadotTxPanel from '../sign-panel/sign_polkadot_tx_panel'
import { LoadingPanel } from '../loading_panel/loading_panel'

// Style
import { LongWrapper } from '../../../stories/style'

export const PendingSignPolkadotTransactionRequestsPanel: React.FC = () => {
  // custom hooks
  const {
    isDisabled,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedRequest,
    signingAccount,
  } = useSignPolkadotTransactionsQueue()

  const { data: network } = useGetNetworkQuery(
    selectedRequest ? selectedRequest.chainId : skipToken,
  )

  // Loading
  if (!network || !selectedRequest || !signingAccount) {
    return <LoadingPanel />
  }

  return (
    <LongWrapper padding='0px'>
      <SignPolkadotTxPanel
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

export default PendingSignPolkadotTransactionRequestsPanel
