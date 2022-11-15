// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { PendingCryptoSendState } from '../../common/reducers/send_crypto_reducer'

export const mockSendCryptoState: PendingCryptoSendState = {
  sendAmount: '',
  toAddress: '',
  toAddressOrUrl: '',
  addressError: undefined,
  addressWarning: undefined,
  selectedSendAsset: undefined,
  showEnsOffchainLookupOptions: false,
  ensOffchainLookupOptions: undefined
}
