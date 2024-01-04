// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

#if DEBUG

class MockSwapService: BraveWalletSwapService {
  func isSwapSupported(_ chainId: String, completion: @escaping (Bool) -> Void) {
    completion(true)
  }

  func transaction(_ params: BraveWallet.SwapTransactionParamsUnion, completion: @escaping (BraveWallet.SwapTransactionUnion?, BraveWallet.SwapErrorUnion?, String) -> Void) {
    completion(.init(zeroExTransaction: .init(price: "", guaranteedPrice: "", to: "", data: "", value: "", gas: "", estimatedGas: "", gasPrice: "", protocolFee: "", minimumProtocolFee: "", buyTokenAddress: "", sellTokenAddress: "", buyAmount: "", sellAmount: "", allowanceTarget: "", sellTokenToEthRate: "", buyTokenToEthRate: "", estimatedPriceImpact: "", sources: [], fees: .init(zeroExFee: nil))), nil, "")
  }
  
  func quote(_ params: BraveWallet.SwapQuoteParams, completion: @escaping (BraveWallet.SwapQuoteUnion?, BraveWallet.SwapErrorUnion?, String) -> Void) {
    completion(.init(zeroExQuote: .init(price: "", guaranteedPrice: "", to: "", data: "", value: "", gas: "", estimatedGas: "", gasPrice: "", protocolFee: "", minimumProtocolFee: "", buyTokenAddress: "", sellTokenAddress: "", buyAmount: "", sellAmount: "", allowanceTarget: "", sellTokenToEthRate: "", buyTokenToEthRate: "", estimatedPriceImpact: "", sources: [], fees: .init(zeroExFee: nil))), nil, "")
  }
  
  func braveFee(_ params: BraveWallet.BraveSwapFeeParams, completion: @escaping (BraveWallet.BraveSwapFeeResponse?, String) -> Void) {
    completion(nil, "Error Message")
  }
}

#endif
