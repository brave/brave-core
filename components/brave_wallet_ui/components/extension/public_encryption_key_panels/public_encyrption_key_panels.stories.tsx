// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockEncryptionKeyRequest,
  mockDecryptRequest, //
} from '../../../stories/mock-data/mock-encryption-key-payload'
import { mockEthAccount } from '../../../common/constants/mocks'

// Components
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import {
  ProvidePublicEncryptionKeyPanel, //
} from './provide_public_encryption_key_panel'
import {
  DecryptMessageRequestPanel, //
} from './decrypt_message_request_panel'

const mockProvideEncryptionKeyRequest = {
  ...mockEncryptionKeyRequest,
  accountId: mockEthAccount.accountId,
}

const mockDecryptMessageRequest = {
  ...mockDecryptRequest,
  accountId: mockEthAccount.accountId,
}

export const _ProvidePublicEncryptionKeyPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <ProvidePublicEncryptionKeyPanel
          payload={mockProvideEncryptionKeyRequest}
        />
      </WalletPanelStory>
    )
  },
}

export const _DecryptMessageRequestPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <DecryptMessageRequestPanel payload={mockDecryptMessageRequest} />
      </WalletPanelStory>
    )
  },
}
export default {
  title: 'Wallet/Panel/Panels/Public Encryption Key Panels',
  parameters: {
    layout: 'centered',
  },
}
