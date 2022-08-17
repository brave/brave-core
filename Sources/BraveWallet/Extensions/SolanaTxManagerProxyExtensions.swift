// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore

extension BraveWalletSolanaTxManagerProxy {
  
  /// Fetches the estimatedTxFee for an array of transaction meta ids.
  func estimatedTxFees(
    for transactionMetaIds: [String],
    completion: @escaping ([String: UInt64]) -> Void
  ) {
    var estimatedTxFees: [String: UInt64] = [:]
    let dispatchGroup = DispatchGroup()
    transactionMetaIds.forEach { txMetaId in
      dispatchGroup.enter()
      estimatedTxFee(txMetaId) { fee, _, _ in
        defer { dispatchGroup.leave() }
        estimatedTxFees[txMetaId] = fee
      }
    }
    dispatchGroup.notify(queue: .main) {
      completion(estimatedTxFees)
    }
  }
  
  /// Fetches the estimatedTxFee for an array of transaction meta ids.
  @MainActor func estimatedTxFees(
    for transactionMetaIds: [String]
  ) async -> [String: UInt64] {
    await withCheckedContinuation { continuation in
      estimatedTxFees(for: transactionMetaIds) { fees in
        continuation.resume(returning: fees)
      }
    }
  }
}
