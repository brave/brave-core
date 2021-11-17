// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

#if DEBUG

class TestSwapController: BraveWalletSwapController {
  func transactionPayload(_ params: BraveWallet.SwapParams, completion: @escaping (Bool, BraveWallet.SwapResponse?, String?) -> Void) {
    completion(false, nil, nil)
  }
  func priceQuote(_ params: BraveWallet.SwapParams, completion: @escaping (Bool, BraveWallet.SwapResponse?, String?) -> Void) {
    completion(false, nil, nil)
  }
}

#endif
