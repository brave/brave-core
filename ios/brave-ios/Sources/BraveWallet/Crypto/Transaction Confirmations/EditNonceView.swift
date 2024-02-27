// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct EditNonceView: View {
  var confirmationStore: TransactionConfirmationStore
  var transaction: BraveWallet.TransactionInfo

  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var nonce = ""
  @State private var isShowingAlert = false

  var body: some View {
    List {
      Section(
        header: Text(Strings.Wallet.editNonceHeader)
          .textCase(.none),
        footer: Text(Strings.Wallet.editNonceFooter)
      ) {
        TextField(Strings.Wallet.editNoncePlaceholder, text: $nonce)
          .keyboardType(.numberPad)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section {
        Button {
          if let value = Int(nonce) {
            let nonceHex = "0x\(String(format: "%02x", value))"
            confirmationStore.editNonce(
              for: transaction,
              nonce: nonceHex
            ) { success in
              if success {
                presentationMode.dismiss()
              } else {
                isShowingAlert = true
              }
            }
          }
        } label: {
          Text(Strings.Wallet.saveButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .disabled(Int(nonce) == nil)
        .frame(maxWidth: .infinity)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.advancedSettingsTransaction)
    .alert(isPresented: $isShowingAlert) {
      Alert(
        title: Text(Strings.Wallet.unknownError),
        message: Text(Strings.Wallet.editTransactionError),
        dismissButton: .default(Text(Strings.Wallet.editTransactionErrorCTA))
      )
    }
    .onAppear {
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        // Most likely a SwiftUI bug. Need add a delay here to render text properly
        setup()
      }
    }
  }

  private func setup() {
    nonce =
      Int(transaction.ethTxNonce.removingHexPrefix, radix: 16).map(String.init)
      ?? transaction.ethTxNonce
  }
}

#if DEBUG
struct EditNonceView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      EditNonceView(
        confirmationStore: .previewStore,
        transaction: .previewConfirmedSend
      )
    }
  }
}
#endif
