// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

#if DEBUG

class TestEthTxController: BraveWalletEthTxController {
  func makeErc721Transfer(fromData from: String, to: String, tokenId: String, contractAddress: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
    completion(false, [])
  }
  
  func setDataForUnapprovedTransaction(_ txMetaId: String, data: [NSNumber], completion: @escaping (Bool) -> Void) {
  }
  
  func speedupOrCancelTransaction(_ txMetaId: String, cancel: Bool, completion: @escaping (Bool, String, String) -> Void) {
  }
  
  func retryTransaction(_ txMetaId: String, completion: @escaping (Bool, String, String) -> Void) {
  }
  
  func approveHardwareTransaction(_ txMetaId: String, completion: @escaping (Bool, String) -> Void) {
  }
  
  func processLedgerSignature(_ txMetaId: String, v: String, r: String, s: String, completion: @escaping (Bool) -> Void) {
  }
  
  func addUnapprovedTransaction(_ txData: BraveWallet.TxData, from: String, completion: @escaping (Bool, String, String) -> Void) {
  }
  
  func addUnapproved1559Transaction(_ txData: BraveWallet.TxData1559, from: String, completion: @escaping (Bool, String, String) -> Void) {
  }
  
  func approveTransaction(_ txMetaId: String, completion: @escaping (Bool) -> Void) {
  }
  
  func rejectTransaction(_ txMetaId: String, completion: @escaping (Bool) -> Void) {
  }
  
  func setGasPriceAndLimitForUnapprovedTransaction(_ txMetaId: String, gasPrice: String, gasLimit: String, completion: @escaping (Bool) -> Void) {
  }
  
  func makeErc20TransferData(_ toAddress: String, amount: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
  }
  
  func makeErc20ApproveData(_ spenderAddress: String, amount: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
  }
  
  func allTransactionInfo(_ from: String, completion: @escaping ([BraveWallet.TransactionInfo]) -> Void) {
  }
  
  func add(_ observer: BraveWalletEthTxControllerObserver) {
  }
  
  func setGasFeeAndLimitForUnapprovedTransaction(_ txMetaId: String, maxPriorityFeePerGas: String, maxFeePerGas: String, gasLimit: String, completion: @escaping (Bool) -> Void) {
  }
  
  func makeErc721Transfer(fromData from: String, to: String, tokenId: String, completion: @escaping (Bool, [NSNumber]) -> Void) {
  }
}

#endif
