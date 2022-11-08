// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Strings
import BraveUI

struct AddCustomAssetView: View {
  @ObservedObject var networkStore: NetworkStore
  var keyringStore: KeyringStore
  @ObservedObject var userAssetStore: UserAssetsStore
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @ObservedObject private var networkSelectionStore: NetworkSelectionStore

  @State private var nameInput = ""
  @State private var addressInput = ""
  @State private var symbolInput = ""
  @State private var decimalsInput = ""
  @State private var logo = ""
  @State private var coingeckoId = ""
  @State private var showError = false
  @State private var showAdvanced = false
  @State private var isPresentingNetworkSelection = false
  
  init(
    networkStore: NetworkStore,
    keyringStore: KeyringStore,
    userAssetStore: UserAssetsStore
  ) {
    self.networkStore = networkStore
    self.keyringStore = keyringStore
    self.userAssetStore = userAssetStore
    self.networkSelectionStore = .init(mode: .formSelection, networkStore: networkStore)
  }

  var body: some View {
    NavigationView {
      Form {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.customTokenNetworkHeader))
        ) {
          HStack {
            Button(action: {
              isPresentingNetworkSelection = true
            }) {
              Text(networkSelectionStore.networkSelectionInForm?.chainName ?? Strings.Wallet.customTokenNetworkButtonTitle)
                .foregroundColor(networkSelectionStore.networkSelectionInForm == nil ? .gray.opacity(0.6) : Color(.braveLabel))
            }
            .disabled(userAssetStore.isSearchingToken)
            Spacer()
            Image(systemName: "chevron.down.circle")
              .foregroundColor(Color(.braveBlurple))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenName))
        ) {
          HStack {
            TextField(Strings.Wallet.enterTokenName, text: $nameInput)
              .autocapitalization(.none)
              .autocorrectionDisabled()
              .disabled(userAssetStore.isSearchingToken)
            if userAssetStore.isSearchingToken && nameInput.isEmpty {
              ProgressView()
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
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
            .autocapitalization(.none)
            .autocorrectionDisabled()
            .disabled(userAssetStore.isSearchingToken)
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.tokenSymbol))
        ) {
          HStack {
            TextField(Strings.Wallet.enterTokenSymbol, text: $symbolInput)
              .autocapitalization(.none)
              .autocorrectionDisabled()
              .disabled(userAssetStore.isSearchingToken)
            if userAssetStore.isSearchingToken && symbolInput.isEmpty {
              ProgressView()
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
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
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section {
          Button(
            action: {
              withAnimation(.easeInOut(duration: 0.25)) {
                showAdvanced.toggle()
              }
            }
          ) {
            VStack {
              HStack {
                Text(Strings.Wallet.addCustomTokenAdvanced)
                  .foregroundColor(.gray)
                Spacer()
                Image("wallet-dismiss", bundle: .module)
                  .renderingMode(.template)
                  .resizable()
                  .foregroundColor(Color(.secondaryBraveLabel))
                  .frame(width: 12, height: 6)
                  .rotationEffect(.degrees(showAdvanced ? 180 : 0))
                  .animation(.default, value: showAdvanced)
              }
              .contentShape(Rectangle())
              Divider()
            }
          }
          .buttonStyle(.plain)
          .accessibilityLabel(Strings.Wallet.addCustomTokenAdvanced)
          .accessibility(addTraits: .isButton)
          .listRowBackground(Color(UIColor.braveGroupedBackground))
          .resetListHeaderStyle()
        }
        if showAdvanced {
          Section(
            header: WalletListHeaderView(title: Text(Strings.Wallet.addCustomTokenIconURL))
          ) {
            HStack {
              TextField(Strings.Wallet.enterTokenIconURL, text: $logo)
                .autocapitalization(.none)
                .autocorrectionDisabled()
                .disabled(userAssetStore.isSearchingToken)
              if userAssetStore.isSearchingToken && decimalsInput.isEmpty {
                ProgressView()
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          Section(
            header: WalletListHeaderView(title: Text(Strings.Wallet.addCustomTokenCoingeckoId))
          ) {
            HStack {
              TextField(Strings.Wallet.enterTokenCoingeckoId, text: $coingeckoId)
                .autocapitalization(.none)
                .autocorrectionDisabled()
                .disabled(userAssetStore.isSearchingToken)
              if userAssetStore.isSearchingToken && decimalsInput.isEmpty {
                ProgressView()
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }
      }
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
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
      .background(
        Color.clear
          .sheet(
            isPresented: $isPresentingNetworkSelection
          ) {
            if let networkSelectionStore = networkSelectionStore {
              NavigationView {
                NetworkSelectionView(
                  keyringStore: keyringStore,
                  networkStore: networkStore,
                  networkSelectionStore: networkSelectionStore
                )
              }
              .accentColor(Color(.braveOrange))
              .navigationViewStyle(.stack)
            }
          }
      )
    }
  }

  private func resignFirstResponder() {
    UIApplication.shared.sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
  }

  private func addCustomToken() {
    userAssetStore.addUserAsset(
      address: addressInput,
      name: nameInput,
      symbol: symbolInput,
      decimals: Int(decimalsInput) ?? Int((networkSelectionStore.networkSelectionInForm?.decimals ?? 18)),
      network: networkSelectionStore.networkSelectionInForm ?? networkStore.selectedChain,
      logo: logo,
      coingeckoId: coingeckoId
    ) { [self] success in
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
    AddCustomAssetView(
      networkStore: .previewStore,
      keyringStore: .previewStore,
      userAssetStore: .previewStore
    )
  }
}
#endif
