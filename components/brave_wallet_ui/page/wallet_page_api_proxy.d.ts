// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { WalletAPIHandler } from '../constants/types'

export interface CreateWalletReturnInfo {
  mnemonic: string
}

export interface GetRecoveryWordsReturnInfo {
  mnemonic: string
}

export default class APIProxy {
  static getInstance: () => APIProxy
  getWalletHandler: () => WalletAPIHandler
  createWallet: (password: string) => Promise<CreateWalletReturnInfo>
  getRecoveryWords: () => Promise<GetRecoveryWordsReturnInfo>
  notifyWalletBackupComplete: () => Promise<void>
}
