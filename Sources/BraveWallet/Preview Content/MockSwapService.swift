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

  func transactionPayload(_ params: BraveWallet.SwapParams, completion: @escaping (BraveWallet.SwapResponse?, BraveWallet.SwapErrorResponse?, String) -> Void) {
    completion(.init(price: "", guaranteedPrice: "", to: "", data: "", value: "", gas: "", estimatedGas: "", gasPrice: "", protocolFee: "", minimumProtocolFee: "", buyTokenAddress: "", sellTokenAddress: "", buyAmount: "", sellAmount: "", allowanceTarget: "", sellTokenToEthRate: "", buyTokenToEthRate: "", estimatedPriceImpact: "", sources: [], fees: .init(zeroExFee: nil)), nil, "")
  }
  
  func priceQuote(_ params: BraveWallet.SwapParams, completion: @escaping (BraveWallet.SwapResponse?, BraveWallet.SwapErrorResponse?, String) -> Void) {
    completion(.init(price: "", guaranteedPrice: "", to: "", data: "", value: "", gas: "", estimatedGas: "", gasPrice: "", protocolFee: "", minimumProtocolFee: "", buyTokenAddress: "", sellTokenAddress: "", buyAmount: "", sellAmount: "", allowanceTarget: "", sellTokenToEthRate: "", buyTokenToEthRate: "", estimatedPriceImpact: "", sources: [], fees: .init(zeroExFee: nil)), nil, "")
  }
  
  func jupiterQuote(_ params: BraveWallet.JupiterQuoteParams, completion: @escaping (BraveWallet.JupiterQuote?, BraveWallet.JupiterErrorResponse?, String) -> Void) {
    completion(nil, nil, "")
  }
  
  func jupiterSwapTransactions(_ params: BraveWallet.JupiterSwapParams, completion: @escaping (Bool, BraveWallet.JupiterSwapTransactions?, String?) -> Void) {
    completion(false, nil, nil)
  }
  
  func hasJupiterFees(forTokenMint mint: String, completion: @escaping (Bool) -> Void) {
    completion(false)
  }
  
  func jupiterSwapTransactions(_ params: BraveWallet.JupiterSwapParams, completion: @escaping (BraveWallet.JupiterSwapTransactions?, BraveWallet.JupiterErrorResponse?, String) -> Void) {
    completion(nil, .init(statusCode: "0", error: "Error", message: "Error Message", isInsufficientLiquidity: false), "")
  }
  
  func braveFee(_ params: BraveWallet.BraveSwapFeeParams, completion: @escaping (BraveWallet.BraveSwapFeeResponse?, String) -> Void) {
    completion(nil, "Error Message")
  }
}

#endif
