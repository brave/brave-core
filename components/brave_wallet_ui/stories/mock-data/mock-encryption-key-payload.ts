// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../constants/types'

import { mockOriginInfo } from './mock-origin-info'
import { mockEthAccount } from './mock-wallet-accounts'

export const mockEncryptionKeyRequest: //
BraveWallet.GetEncryptionPublicKeyRequest = {
  requestId: 'random_id',
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo
}

export const mockDecryptRequest: BraveWallet.DecryptRequest = {
  requestId: 'random_id',
  accountId: mockEthAccount.accountId,
  unsafeMessage: 'This is a test message.',
  originInfo: mockOriginInfo
}
