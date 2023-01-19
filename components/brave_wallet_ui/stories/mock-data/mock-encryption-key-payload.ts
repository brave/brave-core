// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  SerializableDecryptRequest,
  SerializableGetEncryptionPublicKeyRequest
} from '../../constants/types'

import { mockOriginInfo } from './mock-origin-info'

export const mockEncryptionKeyRequest: SerializableGetEncryptionPublicKeyRequest = {
  address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
  originInfo: mockOriginInfo
}

export const mockDecryptRequest: SerializableDecryptRequest = {
  address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
  unsafeMessage: 'This is a test message.',
  originInfo: mockOriginInfo
}
