// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct EditPermissionsView: View {

  let proposedAllowance: String
  @ObservedObject var confirmationStore: TransactionConfirmationStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  @State private var customAllowance: String = ""
  @State private var isShowingAlert = false
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.sizeCategory) private var sizeCategory

  private var activeTransaction: BraveWallet.TransactionInfo {
    confirmationStore.unapprovedTxs.first(where: { $0.id == confirmationStore.activeTransactionId })
      ?? (confirmationStore.unapprovedTxs.first ?? .init())
  }

  private var customAllowanceAmountInWei: String {
    if customAllowance == Strings.Wallet.editPermissionsApproveUnlimited {
      // when user taps 'Set Unlimited' button we updated `customAllowance` to `Strings.Wallet.editPermissionsApproveUnlimited`
      return WalletConstants.maxUInt256
    }

    var decimals: Int = 18
    if case .ethErc20Approve(let details) = confirmationStore.activeParsedTransaction.details {
      decimals = Int(details.token?.decimals ?? networkStore.defaultSelectedChain.decimals)
    }
    let walletAmountFormatter = WalletAmountFormatter(
      decimalFormatStyle: .decimals(precision: decimals)
    )
    let customAllowanceInWei =
      walletAmountFormatter.weiString(from: customAllowance, radix: .hex, decimals: decimals) ?? "0"
    return customAllowanceInWei.addingHexPrefix
  }

  private var accountName: String {
    NamedAddresses.name(
      for: activeTransaction.fromAccountId.address,
      accounts: keyringStore.allAccounts
    )
  }

  init(
    proposedAllowance: String,
    confirmationStore: TransactionConfirmationStore,
    keyringStore: KeyringStore,
    networkStore: NetworkStore
  ) {
    self.proposedAllowance = proposedAllowance
    self.confirmationStore = confirmationStore
    self.keyringStore = keyringStore
    self.networkStore = networkStore
  }

  var body: some View {
    List {
      Section(
        header: Text(
          String.localizedStringWithFormat(
            Strings.Wallet.editPermissionsAllowanceHeader,
            accountName
          )
        )
        .foregroundColor(Color(.secondaryBraveLabel))
        .font(.footnote)
        .resetListHeaderStyle()
        .padding(.vertical)
      ) {
        VStack(alignment: .leading, spacing: 2) {
          Text(Strings.Wallet.editPermissionsProposedAllowanceHeader)
            .foregroundColor(Color(.bravePrimary))
            .fontWeight(.semibold)
          Text("\(confirmationStore.value) \(confirmationStore.symbol)")
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .font(.footnote)
        .padding(.vertical, 6)
      }

      Section(
        header: Text(Strings.Wallet.editPermissionsCustomAllowanceHeader)
          .foregroundColor(Color(.secondaryBraveLabel))
          .font(.footnote)
          .resetListHeaderStyle()
          .padding(.vertical)
      ) {
        HStack {
          TextField(
            "0.0 \(confirmationStore.symbol)",
            text: $customAllowance,
            onEditingChanged: { value in
              if value, customAllowance == Strings.Wallet.editPermissionsApproveUnlimited {
                customAllowance = ""
              }
            }
          )
          .keyboardType(.decimalPad)
          .foregroundColor(Color(.braveLabel))
          if proposedAllowance.caseInsensitiveCompare(WalletConstants.maxUInt256) != .orderedSame {
            Button {
              customAllowance = Strings.Wallet.editPermissionsApproveUnlimited
              resignFirstResponder()
            } label: {
              Text(Strings.Wallet.editPermissionsSetUnlimited)
                .foregroundColor(Color(.braveBlurpleTint))
                .font(.footnote)
            }
          }
        }
      }

      Button {
        confirmationStore.editAllowance(
          transaction: activeTransaction,
          spenderAddress: activeTransaction.txArgs[safe: 0] ?? "",
          amount: customAllowanceAmountInWei
        ) { success in
          if success {
            presentationMode.dismiss()
          } else {
            isShowingAlert = true
          }
        }
      } label: {
        Text(Strings.Wallet.saveButtonTitle)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .frame(maxWidth: .infinity)
      .disabled(customAllowance.isEmpty)
      .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
      .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      .listRowBackground(Color(.braveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.editPermissionsTitle)
    .alert(isPresented: $isShowingAlert) {
      Alert(
        title: Text(Strings.Wallet.unknownError),
        message: Text(Strings.Wallet.editTransactionError),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
  }

  private func resignFirstResponder() {
    UIApplication.shared.sendAction(
      #selector(UIResponder.resignFirstResponder),
      to: nil,
      from: nil,
      for: nil
    )
  }
}

#if DEBUG
struct EditPermissionsView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      EditPermissionsView(
        proposedAllowance: WalletConstants.maxUInt256,
        confirmationStore: .previewStore,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore
      )
    }
  }
}
#endif
