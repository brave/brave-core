// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveUI
import SwiftUI
import struct Shared.Strings
import BigNumber

/// Allows the user to edit the gas fee structure of an regular transaction before confirming it
///
/// If you are looking for editing gas fees for EIP-1559 transactions, see ``EditPriorityFeeView``
struct EditGasFeeView: View {
  /// The transaction being confirmed
  var transaction: BraveWallet.TransactionInfo
  /// The confirmation store to update gas prices on save
  @ObservedObject var confirmationStore: TransactionConfirmationStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var perGasPrice: String = ""
  @State private var gasLimit: String = ""
  
  private func setup() {
    perGasPrice = WeiFormatter.weiToDecimalGwei(transaction.txData.baseData.gasPrice.removingHexPrefix, radix: .hex) ?? "0"
    // Gas limit is already in Gwei…
    gasLimit = {
      guard let value = BDouble(transaction.txData.baseData.gasLimit.removingHexPrefix, radix: 16) else {
        return ""
      }
      if value.denominator == [1] {
        return value.rounded().asString(radix: 10)
      }
      return value.decimalExpansion(precisionAfterDecimalPoint: 2)
    }()
  }
  
  private func save() {
    // Gas limit is already in Gwei, so doesn't need additional conversion other than to hex
    guard let limit = BDouble(gasLimit)?.rounded().asString(radix: 16),
          let gasFeeInWei = WeiFormatter.gweiToWei(perGasPrice, radix: .decimal, outputRadix: .hex) else {
            return
          }
    let hexGasLimit = "0x\(limit)"
    let hexGasFee = "0x\(gasFeeInWei)"
    
    confirmationStore.updateGasFeeAndLimits(
      for: transaction,
      gasPrice: hexGasFee,
      gasLimit: hexGasLimit
    ) { success in
      if success {
        presentationMode.dismiss()
      } else {
         // Show error?
      }
    }
  }
  
  private var isSaveButtonDisabled: Bool {
    guard let gasPrice = BDouble(perGasPrice), gasPrice > 0,
          let limit = BDouble(gasLimit), limit > 0 else {
            return true
          }
    return false
  }
  
  var body: some View {
    List {
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.perGasPriceTitle))
          .osAvailabilityModifiers { content in
            if #available(iOS 15.0, *) {
              content // Padding already applied
            } else {
              content.padding(.top)
            }
          }
      ) {
        TextField("", text: $perGasPrice)
          .keyboardType(.numberPad)
          .foregroundColor(Color(.braveLabel))
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.gasAmountLimit))
      ) {
        TextField("", text: $gasLimit)
          .keyboardType(.numberPad)
          .foregroundColor(Color(.braveLabel))
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        Button(action: save) {
          Text(Strings.Wallet.saveGasFee)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .frame(maxWidth: .infinity)
        .disabled(isSaveButtonDisabled)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.editGasTitle)
    .onAppear {
      // For some reason we need to delay this for SwiftUI to render the text properly…
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        setup()
      }
    }
  }
}
