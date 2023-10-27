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
import {
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token
} from '../../../stories/mock-data/mock-asset-options'

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

export const _ConfirmSwapTransaction = () => {
  return (
    <WalletPanelStory
      walletStateOverride={{
        fullTokenList: [mockBasicAttentionToken, mockBinanceCoinErc20Token],
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
        simulationOptInStatus: 'allowed',
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
              bitcoinAccountIndex: 0
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
          <ConfirmSwapTransaction
            retrySimulation={() => {
              alert('not implemented')
            }}
          />
        </LongWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_ConfirmSwapTransaction.story = {
  name: 'Confirm Swap'
}

export default _ConfirmSwapTransaction
