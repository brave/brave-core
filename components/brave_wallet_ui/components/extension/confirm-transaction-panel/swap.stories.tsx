// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Mocks
import {
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token
} from '../../../stories/mock-data/mock-asset-options'
import { mockNetworks } from '../../../stories/mock-data/mock-networks'
import { mockTransactionInfo } from '../../../stories/mock-data/mock-transaction-info'

// Components
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { LongWrapper } from '../../../stories/style'
import { PanelWrapper } from '../../../panel/style'
import { ConfirmSwapTransaction } from './swap'

export const _ConfirmSwapTransaction = () => {
  const onConfirmTransaction = () => alert('Confirmed Transaction')
  const onRejectTransaction = () => alert('Rejected Transaction')

  return (
    <WalletPanelStory
      walletStateOverride={{
        fullTokenList: [mockBasicAttentionToken, mockBinanceCoinErc20Token],
        networkList: mockNetworks,
        selectedNetwork: mockNetworks[0],
        hasInitialized: true,
        isWalletCreated: true,
        selectedPendingTransaction: {
          ...mockTransactionInfo,
          txType: BraveWallet.TransactionType.ETHSwap,
          txArgs: [
            '0x0D8775F648430679A709E98d2b0Cb6250d2887EF' + // BAT
              'B8c77482e45F1F44dE1745F52C74426C631bDD52', // WETH
            '0x4d12b6295c69ddebd5',
            '0xa34b9dd76c89000'
          ],
          txParams: ['bytes', 'uint256', 'uint256']
        }
      }}
      panelStateOverride={{
        hasInitialized: true
      }}
    >
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ConfirmSwapTransaction onConfirm={onConfirmTransaction} onReject={onRejectTransaction} />
        </LongWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_ConfirmSwapTransaction.story = {
  name: 'Confirm Swap'
}

export default _ConfirmSwapTransaction
