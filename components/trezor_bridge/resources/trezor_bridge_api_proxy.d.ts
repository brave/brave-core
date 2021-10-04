// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import 'gen/brave/components/trezor_bridge/trezor_bridge.mojom-lite.js'

import {
  HardwareWalletAccount
} from './types'

export default class APIProxy {
  static getInstance: () => APIProxy
  onUnlocked: (success: boolean, error: string) => {}
  onAddressesReceived: (success: boolean, accounts: HardwareWalletAccount[], error: string) => {}
  
  /** @return {!trezorBridge.mojom.PageCallbackRouter} */
  getCallbackRouter: () => trezorBridge.mojom.PageCallbackRouter
}
