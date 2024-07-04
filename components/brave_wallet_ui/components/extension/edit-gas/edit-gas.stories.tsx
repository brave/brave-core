// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  UpdateUnapprovedTransactionGasFieldsType //
} from '../../../common/constants/action_types'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import EditGas, { MaxPriorityPanels } from './edit-gas'

// mocks
import {
  mockTransactionInfo //
} from '../../../stories/mock-data/mock-transaction-info'
import { mockGoerli } from '../../../stories/mock-data/mock-networks'
import { PanelWrapper } from '../../../panel/style'

export const _EditGas = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <EditGas
            onCancel={function (): void {
              alert('onCancel')
            }}
            transactionInfo={mockTransactionInfo}
            selectedNetwork={mockGoerli}
            baseFeePerGas={'1'}
            suggestedMaxPriorityFeeChoices={[]}
            suggestedSliderStep={''}
            maxPriorityPanel={MaxPriorityPanels.setCustom}
            updateUnapprovedTransactionGasFields={function (
              payload: UpdateUnapprovedTransactionGasFieldsType
            ): void {
              alert(JSON.stringify(payload, undefined, 2))
            }}
            setSuggestedSliderStep={function (value: string): void {
              alert(value)
            }}
            setMaxPriorityPanel={function (value: MaxPriorityPanels): void {
              alert(value)
            }}
          />
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: EditGas }
