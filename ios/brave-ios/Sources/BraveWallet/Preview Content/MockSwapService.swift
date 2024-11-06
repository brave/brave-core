// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

#if DEBUG

class MockSwapService: BraveWalletSwapService {
  func liFiStatus(
    txHash: String,
    completion: @escaping (BraveWallet.LiFiStatus?, BraveWallet.LiFiError?, String) -> Void
  ) {
    completion(.none, .init(message: "Error", code: .defaultError), "Error")
  }

  func isSwapSupported(chainId: String, completion: @escaping (Bool) -> Void) {
    completion(true)
  }

  func transaction(
    params: BraveWallet.SwapTransactionParamsUnion,
    completion: @escaping (BraveWallet.SwapTransactionUnion?, BraveWallet.SwapErrorUnion?, String)
      -> Void
  ) {
    completion(
      .init(
        zeroExTransaction: .init(
          to: "",
          data: "",
          gas: "",
          gasPrice: "",
          value: ""
        )
      ),
      nil,
      ""
    )
  }

  func quote(
    params: BraveWallet.SwapQuoteParams,
    completion: @escaping (
      BraveWallet.SwapQuoteUnion?, BraveWallet.SwapFees?, BraveWallet.SwapErrorUnion?, String
    ) -> Void
  ) {
    completion(nil, nil, nil, "Error Message")
  }
}

#endif
