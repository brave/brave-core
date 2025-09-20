// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// mocks
import { mockSignCardanoTransactionRequest } from '../../../common/constants/mocks'

// utils

// components
import {
  WalletPanelStory, //
  WalletPanelStoryProps,
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

import PendingSignCardanoTransactionRequestsPanel from './pending_sign_cardano_tx_requests_panel'

const storyContextProps: WalletPanelStoryProps = {
  walletApiDataOverrides: {
    signCardanoTransactionRequests: [
      mockSignCardanoTransactionRequest,
      mockSignCardanoTransactionRequest,
    ],
  },
  uiStateOverride: {},
}

export const _PendingSignCardanoTransactionRequestsPanel = {
  render: () => {
    return (
      <WalletPanelStory {...storyContextProps}>
        <PendingSignCardanoTransactionRequestsPanel />
      </WalletPanelStory>
    )
  },
}

export default {
  parameters: {
    layout: 'centered',
  },
  title: 'Wallet/Panel/Panels/Sign Transaction',
  component: PendingSignCardanoTransactionRequestsPanel,
}
