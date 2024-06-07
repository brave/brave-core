// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

extension BraveWalletSolanaTxManagerProxy {

  /// Fetches the solanaTxFeeEstimation for an array of transactions
  @MainActor func solanaTxFeeEstimations(
    for transactions: [BraveWallet.TransactionInfo]
  ) async -> [String: UInt64] {
    return await withTaskGroup(
      of: [String: UInt64].self,
      body: { @MainActor group in
        for tx in transactions {
          group.addTask { @MainActor in
            let (fee, _, _) = await self.solanaTxFeeEstimation(chainId: tx.chainId, txMetaId: tx.id)
            let priorityFee =
              (UInt64(fee.computeUnits) * fee.feePerComputeUnit)
              / BraveWallet.MicroLamportsPerLamport
            let totalFee = fee.baseFee + priorityFee
            return [tx.id: totalFee]
          }
        }
        var estimatedFees: [String: UInt64] = [:]
        for await fee in group {
          estimatedFees.merge(with: fee)
        }
        return estimatedFees
      }
    )
  }
}
