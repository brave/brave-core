// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import BraveUI
import DesignSystem
import Strings
import SwiftUI

/// Allows the user to edit the gas fee structure of an EIP-1559 transaction before confirming it
///
/// If you are looking for editing gas fees for standard transactions, see ``EditGasFeeView``
struct EditPriorityFeeView: View {
  /// The transaction being confirmed
  var transaction: BraveWallet.TransactionInfo
  /// A gas estimation for the transaction
  ///
  /// This is explicitly passed in to ensure there is a valid gas estimation before attempting to edit gas.
  /// Note: This gas estimation for eip 1559 comes from AssetRaitoService.gasOracle
  /// Use this instead of `transaction.txData.gasEstimation`
  var gasEstimation: BraveWallet.GasEstimation1559
  /// The confirmation store to update gas prices on save
  @ObservedObject var confirmationStore: TransactionConfirmationStore

  @Environment(\.presentationMode) @Binding private var presentationMode

  private enum GasFeeKind: Hashable {
    case low, optimal, high, custom
  }

  @State private var gasFeeKind: GasFeeKind = .optimal
  @State private var gasLimit: String = ""
  @State private var maximumGasPrice: String = ""
  @State private var maximumTipPrice: String = ""
  @State private var baseInGwei: String = ""
  @State private var isShowingAlert: Bool = false

  private func setup() {
    let selectedMaxPrice = transaction.txDataUnion.ethTxData1559?.maxFeePerGas ?? ""
    let selectedMaxTip = transaction.txDataUnion.ethTxData1559?.maxPriorityFeePerGas ?? ""

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
    maximumTipPrice =
      WalletAmountFormatter.weiToDecimalGwei(selectedMaxTip.removingHexPrefix, radix: .hex) ?? "0"
    maximumGasPrice =
      WalletAmountFormatter.weiToDecimalGwei(selectedMaxPrice.removingHexPrefix, radix: .hex) ?? "0"
    baseInGwei =
      WalletAmountFormatter.weiToDecimalGwei(
        gasEstimation.baseFeePerGas.removingHexPrefix,
        radix: .hex
      )
      ?? "0"

    // Comparing from high to low as sometimes avg/slow fees are the same
    if selectedMaxPrice == gasEstimation.fastMaxFeePerGas
      && selectedMaxTip == gasEstimation.fastMaxPriorityFeePerGas
    {
      gasFeeKind = .high
    } else if selectedMaxPrice == gasEstimation.avgMaxFeePerGas
      && selectedMaxTip == gasEstimation.avgMaxPriorityFeePerGas
    {
      gasFeeKind = .optimal
    } else if selectedMaxPrice == gasEstimation.slowMaxFeePerGas
      && selectedMaxTip == gasEstimation.slowMaxPriorityFeePerGas
    {
      gasFeeKind = .low
    } else {
      gasFeeKind = .custom
    }
  }

  private func save() {
    // See `isSaveButtonDisabled` for validation
    var hexGasLimit = transaction.ethTxGasLimit
    let hexGasFee: String
    let hexGasTip: String
    switch gasFeeKind {
    case .low:
      hexGasFee = gasEstimation.slowMaxFeePerGas
      hexGasTip = gasEstimation.slowMaxPriorityFeePerGas
    case .optimal:
      hexGasFee = gasEstimation.avgMaxFeePerGas
      hexGasTip = gasEstimation.avgMaxPriorityFeePerGas
    case .high:
      hexGasFee = gasEstimation.fastMaxFeePerGas
      hexGasTip = gasEstimation.fastMaxPriorityFeePerGas
    case .custom:
      // Gas limit is already in Gwei, so doesn't need additional conversion other than to hex
      guard let limit = BDouble(gasLimit)?.rounded().asString(radix: 16),
        let gasFee = WalletAmountFormatter.gweiToWei(
          maximumGasPrice,
          radix: .decimal,
          outputRadix: .hex
        ),
        let gasTip = WalletAmountFormatter.gweiToWei(
          maximumTipPrice,
          radix: .decimal,
          outputRadix: .hex
        )
      else {
        // Show error?
        return
      }
      hexGasLimit = "0x\(limit)"
      hexGasFee = "0x\(gasFee)"
      hexGasTip = "0x\(gasTip)"
    }

    confirmationStore.updateGasFeeAndLimits(
      for: transaction,
      maxPriorityFeePerGas: hexGasTip,
      maxFeePerGas: hexGasFee,
      gasLimit: hexGasLimit
    ) { success in
      if success {
        presentationMode.dismiss()
      } else {
        isShowingAlert = true
      }
    }
  }

  private var calculatedMaximumFee: String {
    let formatter = WalletAmountFormatter(
      decimalFormatStyle: .gasFee(limit: gasLimit, radix: .decimal)
    )
    let gasFeeInWei: String
    switch gasFeeKind {
    case .low:
      gasFeeInWei = gasEstimation.slowMaxFeePerGas
    case .optimal:
      gasFeeInWei = gasEstimation.avgMaxFeePerGas
    case .high:
      gasFeeInWei = gasEstimation.fastMaxFeePerGas
    case .custom:
      gasFeeInWei =
        WalletAmountFormatter.gweiToWei(maximumGasPrice, radix: .decimal, outputRadix: .hex) ?? ""
    }
    let proposedGasValue =
      formatter.decimalString(for: gasFeeInWei.removingHexPrefix, radix: .hex, decimals: 18) ?? ""
    let proposedGasFiat =
      confirmationStore.currencyFormatter.formatAsFiat(
        confirmationStore.gasAssetRatio * (Double(proposedGasValue) ?? 0.0)
      ) ?? "–"
    return "\(proposedGasFiat) (\(proposedGasValue) \(confirmationStore.gasSymbol))"
  }

  private var isSaveButtonDisabled: Bool {
    if case .custom = gasFeeKind {
      // Validate data
      guard let gasLimitValue = BDouble(gasLimit), gasLimitValue > 0,
        let basePrice = BDouble(baseInGwei),
        let maxTipValue = BDouble(maximumTipPrice), maxTipValue > 0,
        let maxFeeValue = BDouble(maximumGasPrice), maxFeeValue > basePrice
      else {
        return true
      }
    }
    return false
  }

  private var maximumPriorityFeeBinding: Binding<String> {
    // We have to setup a custom `Binding` instead of using `onChange` so that `maximumGasPrice` is not
    // altered during setup. If a user edits the `maximumGasPrice` themselves and saves it, when they come
    // back to the edit gas view, it needs to show that value, not the one computed below based on the
    // `maximumTipPrice`
    .init {
      maximumTipPrice
    } set: { value in
      maximumTipPrice = value
      if let base = BDouble(baseInGwei),
        let tip = BDouble(value)
      {
        maximumGasPrice = (floor(base) + tip).decimalExpansion(precisionAfterDecimalPoint: 2)
      }
    }
  }

  var body: some View {
    List {
      Section(
        header: Text(Strings.Wallet.gasFeeDisclaimer)
          .foregroundColor(Color(.secondaryBraveLabel))
          .font(.footnote)
          .resetListHeaderStyle()
          .padding(.vertical)
      ) {
        Picker(selection: $gasFeeKind.animation(.default)) {
          Text(Strings.Wallet.gasFeePredefinedLimitLow).tag(GasFeeKind.low)
          Text(Strings.Wallet.gasFeePredefinedLimitOptimal).tag(GasFeeKind.optimal)
          Text(Strings.Wallet.gasFeePredefinedLimitHigh).tag(GasFeeKind.high)
          Text(Strings.Wallet.gasFeeCustomOption).tag(GasFeeKind.custom)
        } label: {
          EmptyView()
        }
        .accentColor(Color(.braveBlurpleTint))
        .pickerStyle(.inline)
        .foregroundColor(Color(.braveLabel))
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      if gasFeeKind == .custom {
        Section(
          header: VStack {
            HStack {
              Text(Strings.Wallet.gasCurrentBaseFee)
              Spacer()
              Text("\(baseInGwei) Gwei")
            }
          }
          .font(.headline)
          .foregroundColor(Color(.braveLabel))
          .padding(.top)
          .padding(.bottom, 12)
          .resetListHeaderStyle()
        ) {
          Group {
            VStack(alignment: .leading, spacing: 4) {
              Text(Strings.Wallet.gasAmountLimit)
                .foregroundColor(Color(.bravePrimary))
                .font(.footnote.weight(.semibold))
              TextField("", text: $gasLimit)
                .keyboardType(.numberPad)
                .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 6)
            VStack(alignment: .leading, spacing: 4) {
              Text(Strings.Wallet.perGasTipLimit)
                .font(.footnote.weight(.semibold))
                .foregroundColor(Color(.bravePrimary))
              TextField("", text: maximumPriorityFeeBinding)
                .keyboardType(.numberPad)
                .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 6)
            VStack(alignment: .leading, spacing: 4) {
              Text(Strings.Wallet.perGasPriceLimit)
                .font(.footnote.weight(.semibold))
                .foregroundColor(Color(.bravePrimary))
              TextField("", text: $maximumGasPrice)
                .keyboardType(.numberPad)
                .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 6)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      Section {
        VStack {
          Text(Strings.Wallet.maximumGasFee)
            .fontWeight(.regular)
          Text("~\(calculatedMaximumFee)")
            .bold()
        }
        .foregroundColor(Color(.braveLabel))
        .font(.headline)
        .frame(maxWidth: .infinity)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
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
    .navigationTitle(Strings.Wallet.maxPriorityFeeTitle)
    .alert(isPresented: $isShowingAlert) {
      Alert(
        title: Text(Strings.Wallet.unknownError),
        message: Text(Strings.Wallet.editTransactionError),
        dismissButton: .default(Text(Strings.Wallet.editTransactionErrorCTA))
      )
    }
    .onAppear(perform: setup)
  }
}

#if DEBUG
struct EditPriorityFeeView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      EditPriorityFeeView(
        transaction: .previewConfirmedSend,
        gasEstimation: .init(),
        confirmationStore: .previewStore
      )
    }
    .previewColorSchemes()
  }
}
#endif
