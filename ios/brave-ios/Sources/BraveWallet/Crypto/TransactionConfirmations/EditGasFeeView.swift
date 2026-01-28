// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import BraveUI
import DesignSystem
import Foundation
import Strings
import SwiftUI

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
  @State private var isShowingAlert: Bool = false

  private func setup() {
    perGasPrice =
      WalletAmountFormatter.weiToDecimalGwei(
        transaction.ethTxGasPrice.removingHexPrefix,
        radix: .hex
      ) ?? "0"
    // Gas limit is already in Gwei…
    gasLimit = {
      guard let value = BDouble(transaction.ethTxGasLimit.removingHexPrefix, radix: 16) else {
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
      let gasFeeInWei = WalletAmountFormatter.gweiToWei(
        perGasPrice,
        radix: .decimal,
        outputRadix: .hex
      )
    else {
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
        isShowingAlert = true
      }
    }
  }

  private var isSaveButtonDisabled: Bool {
    guard let gasPrice = BDouble(perGasPrice), gasPrice > 0,
      let limit = BDouble(gasLimit), limit > 0
    else {
      return true
    }
    return false
  }

  var body: some View {
    List {
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.perGasPriceTitle))
      ) {
        TextField("", text: $perGasPrice)
          .keyboardType(.numberPad)
          .foregroundColor(Color(.braveLabel))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.gasAmountLimit))
      ) {
        TextField("", text: $gasLimit)
          .keyboardType(.numberPad)
          .foregroundColor(Color(.braveLabel))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section {
        Button(action: save) {
          Text(Strings.Wallet.saveButtonTitle)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .frame(maxWidth: .infinity)
        .disabled(isSaveButtonDisabled)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.editGasTitle)
    .alert(isPresented: $isShowingAlert) {
      Alert(
        title: Text(Strings.Wallet.unknownError),
        message: Text(Strings.Wallet.editTransactionError),
        dismissButton: .default(Text(Strings.Wallet.editTransactionErrorCTA))
      )
    }
    .onAppear {
      // For some reason we need to delay this for SwiftUI to render the text properly…
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        setup()
      }
    }
  }
}
