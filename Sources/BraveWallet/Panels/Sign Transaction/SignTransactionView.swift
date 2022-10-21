// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct SignTransactionView: View {
  enum Request {
    case signTransaction(BraveWallet.SignTransactionRequest)
    case signAllTransactions(BraveWallet.SignAllTransactionsRequest)
    
    var instructions: [[BraveWallet.SolanaInstruction]] {
      switch self {
      case let .signTransaction(request):
        if let instructions = request.txData.solanaTxData?.instructions {
          return [instructions]
        }
        return []
      case let .signAllTransactions(request):
        return request.txDatas.map { $0.solanaTxData?.instructions ?? [] }
      }
    }
  }
  
  var request: Request
  var cryptoStore: CryptoStore
  var onDismiss: () -> Void
  
  var navigationTitle: String {
    switch request {
    case .signTransaction:
      return Strings.Wallet.signTransactionTitle
    case .signAllTransactions:
      return Strings.Wallet.signAllTransactionsTitle
    }
  }
  
  private func instructionsDisplayString() -> String {
    request.instructions
      .map { instructionsForOneTx in
        instructionsForOneTx
          .map { TransactionParser.parseSolanaInstruction($0).toString }
          .joined(separator: "\n\n") // separator between each instruction
      }
      .joined(separator: "\n\n\n\n") // separator between each transaction
  }
  
  var body: some View { // TODO: Issue #6005 - SignTransaction / SignAllTransactions panel
    VStack {
      StaticTextView(text: instructionsDisplayString(), isMonospaced: false)
        .frame(maxWidth: .infinity)
        .frame(height: 200)
        .background(Color(.tertiaryBraveGroupedBackground))
        .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
        .padding()
        .background(Color(.secondaryBraveGroupedBackground))
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
      Button(action: {
        switch request {
        case let .signTransaction(request):
          cryptoStore.handleWebpageRequestResponse(
            .signTransaction(approved: false, id: request.id, signature: nil, error: "User rejected the request")
          )
        case let .signAllTransactions(request):
          cryptoStore.handleWebpageRequestResponse(
            .signAllTransactions(approved: false, id: request.id, signatures: nil, error: "User rejected the request")
          )
        }
        onDismiss()
      }) {
        Text(Strings.Wallet.rejectTransactionButtonTitle)
      }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
    }
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Text(navigationTitle))
  }
}
