// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Web

extension TabDataValues {
  // Cardano Provider - for enable() and isEnabled()
  private struct CardanoProviderKey: TabDataKey {
    static var defaultValue: BraveWalletCardanoProvider?
  }

  var walletCardanoProvider: BraveWalletCardanoProvider? {
    get { self[CardanoProviderKey.self] }
    set { self[CardanoProviderKey.self] = newValue }
  }

  // Cardano API - returned by enable(), used for CIP-30 operations
  private struct CardanoApiKey: TabDataKey {
    static var defaultValue: BraveWalletCardanoApi?
  }

  var walletCardanoApi: BraveWalletCardanoApi? {
    get { self[CardanoApiKey.self] }
    set { self[CardanoApiKey.self] = newValue }
  }
}
