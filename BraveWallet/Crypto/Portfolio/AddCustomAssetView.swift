// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Shared

struct AddCustomAssetView: View {
  @ObservedObject var userAssetStore: UserAssetsStore
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var nameInput = ""
  @State private var addressInput = ""
  @State private var symbolInput = ""
  @State private var decimalsInput = ""
  @State private var showError = false
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenName))
        ) {
          HStack {
            TextField(Strings.Wallet.enterTokenName, text: $nameInput)
              .disabled(userAssetStore.isSearchingToken)
            if userAssetStore.isSearchingToken && nameInput.isEmpty {
              ProgressView()
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenContractAddress))
        ) {
          TextField(Strings.Wallet.enterContractAddress, text: $addressInput)
            .onChange(of: addressInput) { newValue in
              if !newValue.isEmpty, newValue.isETHAddress {
                userAssetStore.tokenInfo(by: newValue) { token in
                  if let token = token, !token.isErc721 {
                    if nameInput.isEmpty {
                      nameInput = token.name
                    }
                    if symbolInput.isEmpty {
                      symbolInput = token.symbol
                    }
                    if decimalsInput.isEmpty {
                      decimalsInput = "\(token.decimals)"
                    }
                  }
                }
              }
            }
            .disabled(userAssetStore.isSearchingToken)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenSymbol))
        ) {
          HStack {
            TextField(Strings.Wallet.enterTokenSymbol, text: $symbolInput)
              .disabled(userAssetStore.isSearchingToken)
            if userAssetStore.isSearchingToken && symbolInput.isEmpty {
              ProgressView()
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.decimalsPrecision))
        ) {
          HStack {
            TextField(NumberFormatter().string(from: NSNumber(value: 0)) ?? "0", text: $decimalsInput)
              .keyboardType(.numberPad)
              .disabled(userAssetStore.isSearchingToken)
            if userAssetStore.isSearchingToken && decimalsInput.isEmpty {
              ProgressView()
            }
          }
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
    let token = BraveWallet.BlockchainToken(contractAddress: addressInput,
                                            name: nameInput,
                                            logo: "",
                                            isErc20: true,
                                            isErc721: false,
                                            symbol: symbolInput,
                                            decimals: Int32(decimalsInput) ?? 18,
                                            visible: true,
                                            tokenId: "",
                                            coingeckoId: "")
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
      AddCustomAssetView(userAssetStore: UserAssetsStore(walletService: MockBraveWalletService(),
                                                         blockchainRegistry: MockBlockchainRegistry(),
                                                         rpcService: MockJsonRpcService(),
                                                         assetRatioService: MockAssetRatioService()))
    }
}
#endif
