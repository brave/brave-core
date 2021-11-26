// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Shared

struct AddCustomAssetView: View {
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var nameInput = ""
  @State private var addressInput = ""
  @State private var symbolInput = ""
  @State private var decimalsInput = ""
  @State private var showError = false
  
  var userAssetStore: UserAssetsStore
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenName))
        ) {
          TextField(Strings.Wallet.enterTokenName, text: $nameInput)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenContractAddress))
        ) {
          TextField(Strings.Wallet.enterContractAddress, text: $addressInput)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenSymbol))
        ) {
          TextField(Strings.Wallet.enterTokenSymbol, text: $symbolInput)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.decimalsPrecision))
        ) {
          TextField(NumberFormatter().string(from: NSNumber(value: 0)) ?? "0", text: $decimalsInput)
            .keyboardType(.numberPad)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationTitle(Strings.Wallet.customTokenTitle)
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
          ToolbarItemGroup(placement: .cancellationAction) {
            Button(action: {
              presentationMode.dismiss()
            }) {
              Text(Strings.cancelButtonTitle)
                .foregroundColor(Color(.braveOrange))
            }
          }
          ToolbarItemGroup(placement: .navigationBarTrailing) {
            Button(action: {
              resignFirstResponder()
              addCustomToken()
            }) {
              Text(Strings.Wallet.add)
                .foregroundColor(Color(.braveOrange))
            }
          }
        }
        .alert(isPresented: $showError) {
          Alert(
            title: Text(Strings.Wallet.addCustomTokenErrorTitle),
            message: Text(Strings.Wallet.addCustomTokenErrorMessage),
            dismissButton: .default(Text(Strings.OKString))
          )
        }
    }
  }
  
  private func resignFirstResponder() {
      UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
  }
  
  private func addCustomToken() {
    let token = BraveWallet.ERCToken(contractAddress: addressInput,
                                     name: nameInput,
                                     logo: "",
                                     isErc20: true,
                                     isErc721: false,
                                     symbol: symbolInput,
                                     decimals: Int32(symbolInput) ?? 18,
                                     visible: true,
                                     tokenId: "")
    userAssetStore.addUserAsset(token: token) { [self] success in
      if success {
        presentationMode.dismiss()
      } else {
        showError = true
      }
    }
  }
}

#if DEBUG
struct AddCustomAssetView_Previews: PreviewProvider {
    static var previews: some View {
      AddCustomAssetView(userAssetStore: UserAssetsStore(walletService: TestBraveWalletService(),
                                                         tokenRegistry: TestTokenRegistry(),
                                                         rpcController: TestEthJsonRpcController()))
    }
}
#endif
