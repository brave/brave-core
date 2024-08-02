// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Mocks
import {
  mockTransactionInfo //
} from '../../../stories/mock-data/mock-transaction-info'

// Components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { LongWrapper } from '../../../stories/style'
import { PanelWrapper } from '../../../panel/style'
import { ConfirmSwapTransaction } from './swap'

// Utils
import {
  deserializeTransaction //
} from '../../../utils/model-serialization-utils'

export const _ConfirmSwapTransaction = {
  render: () => {
    return (
      <WalletPanelStory
        walletStateOverride={{
          hasInitialized: true,
          isWalletCreated: true
        }}
        uiStateOverride={{
          selectedPendingTransactionId: mockTransactionInfo.id
        }}
        panelStateOverride={{
          hasInitialized: true
        }}
        walletApiDataOverrides={{
          simulationOptInStatus: BraveWallet.BlowfishOptInStatus.kAllowed,
          evmSimulationResponse: {
            error: {
              humanReadableError: 'Simulation failed',
              kind: BraveWallet.BlowfishEVMErrorKind.kSimulationFailed
            },
            expectedStateChanges: [],
            action: BraveWallet.BlowfishSuggestedAction.kWarn,
            warnings: []
          },
          transactionInfos: [
            deserializeTransaction({
              ...mockTransactionInfo,
              txStatus: BraveWallet.TransactionStatus.Unapproved
            })
          ],
          accountInfos: [
            {
              accountId: {
                address: mockTransactionInfo.fromAddress || '',
                coin: BraveWallet.CoinType.ETH,
                keyringId: BraveWallet.KeyringId.kDefault,
                kind: BraveWallet.AccountKind.kDerived,
                uniqueKey: '',
                accountIndex: 0
              },
              address: mockTransactionInfo.fromAddress || '',
              hardware: undefined,
              name: '1'
            }
          ]
        }}
      >
        <PanelWrapper isLonger={true}>
          <LongWrapper>
            <ConfirmSwapTransaction />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: ConfirmSwapTransaction }
