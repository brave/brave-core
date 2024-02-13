// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

#if DEBUG

class MockEthTxManagerProxy: BraveWalletEthTxManagerProxy {
  func setGasPriceAndLimitForUnapprovedTransaction(_ chainId: String, txMetaId: String, gasPrice: String, gasLimit: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func setGasFeeAndLimitForUnapprovedTransaction(_ chainId: String, txMetaId: String, maxPriorityFeePerGas: String, maxFeePerGas: String, gasLimit: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func setDataForUnapprovedTransaction(_ chainId: String, txMetaId: String, data: [NSNumber], completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func setNonceForUnapprovedTransaction(_ chainId: String, txMetaId: String, nonce: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func makeErc20TransferData(_ toAddress: String, amount: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
    completion(true, .init())
  }

  func makeErc20ApproveData(_ spenderAddress: String, amount: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
    completion(false, .init())
  }

  func makeErc721Transfer(fromData from: String, to: String, tokenId: String, contractAddress: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
    completion(false, .init())
  }

  func gasEstimation1559(_ chainId: String, completion: @escaping (BraveWallet.GasEstimation1559?) -> Void) {
    completion(nil)
  }

  func nonce(forHardwareTransaction chainId: String, txMetaId: String, completion: @escaping (String?) -> Void) {
    completion(nil)
  }

  func processHardwareSignature(_ chainId: String, txMetaId: String, v: String, r: String, s: String, completion: @escaping (Bool, BraveWallet.ProviderError, String) -> Void) {
    completion(false, .internalError, "Error Message")
  }
  
  func makeErc1155Transfer(fromData from: String, to: String, tokenId: String, value: String, contractAddress: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
    completion(false, [])
  }

  func makeFilForwarderTransferData(_ toAddressFil: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
    completion(false, [])
  }
}

#endif
