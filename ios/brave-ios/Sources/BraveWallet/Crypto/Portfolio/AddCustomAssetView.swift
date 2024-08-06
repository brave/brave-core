// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Strings
import SwiftUI

struct AddCustomAssetView: View {
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var networkSelectionStore: NetworkSelectionStore
  var keyringStore: KeyringStore
  @ObservedObject var userAssetStore: UserAssetsStore
  var tokenNeedsTokenId: BraveWallet.BlockchainToken?
  var supportedTokenTypes: [TokenType] = [.token, .nft]

  @Environment(\.presentationMode) @Binding private var presentationMode

  enum TokenType: Int, Identifiable {
    case token
    case nft

    var id: Self { self }

    var title: String {
      switch self {
      case .token:
        return Strings.Wallet.addCustomTokenTitle
      case .nft:
        return Strings.Wallet.addCustomNFTTitle
      }
    }
  }

  @State private var selectedTokenType: TokenType = .token
  @State private var nameInput = ""
  @State private var addressInput = ""
  @State private var symbolInput = ""
  @State private var decimalsInput = ""
  @State private var tokenId = ""
  @State private var logo = ""
  @State private var coingeckoId = ""
  @State private var showError = false
  @State private var showAdvanced = false
  @State private var isPresentingNetworkSelection = false

  private var addButtonDisabled: Bool {
    switch selectedTokenType {
    case .token:
      return nameInput.isEmpty || symbolInput.isEmpty || decimalsInput.isEmpty
        || addressInput.isEmpty || networkSelectionStore.networkSelectionInForm == nil
        || (networkSelectionStore.networkSelectionInForm?.coin != .sol
          && !addressInput.isETHAddress)
    case .nft:
      return nameInput.isEmpty || symbolInput.isEmpty
        || (networkSelectionStore.networkSelectionInForm?.coin != .sol && tokenId.isEmpty)
        || addressInput.isEmpty || networkSelectionStore.networkSelectionInForm == nil
        || (networkSelectionStore.networkSelectionInForm?.coin != .sol
          && !addressInput.isETHAddress)
    }
  }

  private var showTokenID: Bool {
    if let customAssetNetwork = networkSelectionStore.networkSelectionInForm,
      customAssetNetwork.coin == .sol
    {
      return false
    }
    return true
  }

  var body: some View {
    NavigationView {
      Form {
        if supportedTokenTypes.count != 1 {
          Section {
          } header: {
            Picker("", selection: $selectedTokenType) {
              ForEach(supportedTokenTypes) { type in
                Text(type.title)
              }
            }
            .pickerStyle(.segmented)
          }
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.customTokenNetworkHeader))
        ) {
          HStack {
            Button {
              isPresentingNetworkSelection = true
            } label: {
              Text(
                networkSelectionStore.networkSelectionInForm?.chainName
                  ?? Strings.Wallet.customTokenNetworkButtonTitle
              )
              .foregroundColor(
                networkSelectionStore.networkSelectionInForm == nil
                  ? Color(braveSystemName: .textDisabled) : Color(.braveLabel)
              )
            }
            Spacer()
            Image(systemName: "chevron.down.circle")
              .foregroundColor(Color(.braveBlurpleTint))
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(
            title: networkSelectionStore.networkSelectionInForm?.coin == .sol
              ? Text(Strings.Wallet.tokenMintAddress) : Text(Strings.Wallet.tokenAddress)
          )
        ) {
          TextField(Strings.Wallet.enterAddress, text: $addressInput)
            .onChange(of: addressInput) { newValue in
              guard !newValue.isEmpty,
                let network = networkSelectionStore.networkSelectionInForm
              else { return }
              userAssetStore.tokenInfo(address: newValue, chainId: network.chainId) { token in
                guard let token else { return }
                if nameInput.isEmpty {
                  nameInput = token.name
                }
                if symbolInput.isEmpty {
                  symbolInput = token.symbol
                }
                if !token.isErc721, !token.isNft, decimalsInput.isEmpty {
                  decimalsInput = "\(token.decimals)"
                }
              }
            }
            .autocapitalization(.none)
            .autocorrectionDisabled()
            .disabled(userAssetStore.isSearchingToken)
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
        switch selectedTokenType {
        case .token:
          Section(
            header: WalletListHeaderView(title: Text(Strings.Wallet.decimalsPrecision))
          ) {
            HStack {
              TextField(
                NumberFormatter().string(from: NSNumber(value: 0)) ?? "0",
                text: $decimalsInput
              )
              .keyboardType(.numberPad)
              .disabled(userAssetStore.isSearchingToken)
              if userAssetStore.isSearchingToken && decimalsInput.isEmpty {
                ProgressView()
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          Section {
            Button {
              withAnimation(.easeInOut(duration: 0.25)) {
                showAdvanced.toggle()
              }
            } label: {
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
              }
              .listRowBackground(Color(.secondaryBraveGroupedBackground))
            }
          }
        case .nft:
          if showTokenID {
            Section(
              header: WalletListHeaderView(title: Text(Strings.Wallet.addCustomTokenId))
            ) {
              HStack {
                TextField(Strings.Wallet.enterTokenId, text: $tokenId)
                  .keyboardType(.numberPad)
              }
              .listRowBackground(Color(.secondaryBraveGroupedBackground))
            }
          }
        }
      }
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .onChange(
        of: selectedTokenType,
        perform: { _ in
          guard tokenNeedsTokenId == nil else { return }
          resignFirstResponder()
          clearInput()
        }
      )
      .navigationTitle(Strings.Wallet.customTokenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            presentationMode.dismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
        if userAssetStore.isAddingAsset {
          ToolbarItemGroup(placement: .navigationBarTrailing) {
            ProgressView()
          }
        } else {
          ToolbarItemGroup(placement: .navigationBarTrailing) {
            Button {
              resignFirstResponder()
              addCustomToken()
            } label: {
              Text(Strings.Wallet.add)
            }
            .disabled(addButtonDisabled)
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
            isPresented: Binding(
              get: { isPresentingNetworkSelection },
              set: {
                isPresentingNetworkSelection = $0
              }
            )
          ) {
            NavigationView {
              NetworkSelectionView(
                keyringStore: keyringStore,
                networkStore: networkStore,
                networkSelectionStore: networkSelectionStore,
                networkSelectionType: .addCustomAsset
              )
            }
            .accentColor(Color(.braveBlurpleTint))
            .navigationViewStyle(.stack)
          }
      )
      .onAppear {
        if supportedTokenTypes.count == 1, let tokenType = supportedTokenTypes.first,
          selectedTokenType != tokenType
        {
          Task { @MainActor in
            selectedTokenType = tokenType
          }
        }
        if case .nft = selectedTokenType, let token = tokenNeedsTokenId {
          Task { @MainActor in
            networkSelectionStore.networkSelectionInForm = await userAssetStore.networkInfo(
              by: token.chainId,
              coin: token.coin
            )
            nameInput = token.name
            symbolInput = token.symbol
            addressInput = token.contractAddress
          }
        }
      }
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

  private func clearInput() {
    nameInput = ""
    addressInput = ""
    symbolInput = ""
    decimalsInput = ""
    tokenId = ""
    logo = ""
    coingeckoId = ""
    networkSelectionStore.networkSelectionInForm = nil
  }

  private func addCustomToken() {
    Task { @MainActor in
      let network =
        networkSelectionStore.networkSelectionInForm ?? networkStore.defaultSelectedChain
      let token: BraveWallet.BlockchainToken
      switch selectedTokenType {
      case .token:
        token = BraveWallet.BlockchainToken(
          contractAddress: addressInput,
          name: nameInput,
          logo: logo,
          isCompressed: false,
          isErc20: network.coin != .sol,
          isErc721: false,
          isErc1155: false,
          splTokenProgram: .unknown,
          isNft: false,
          isSpam: false,
          symbol: symbolInput,
          decimals: Int32(decimalsInput)
            ?? Int32((networkSelectionStore.networkSelectionInForm?.decimals ?? 18)),
          visible: true,
          tokenId: "",
          coingeckoId: coingeckoId,
          chainId: network.chainId,
          coin: network.coin
        )
      case .nft:
        var tokenIdToHex = ""
        if let tokenIdValue = Int16(tokenId) {
          tokenIdToHex = "0x\(String(format: "%02x", tokenIdValue))"
        }
        if let knownERC721Token = tokenNeedsTokenId {
          knownERC721Token.tokenId = tokenIdToHex
          if let userSelectedNetworkId = networkSelectionStore.networkSelectionInForm?.chainId {
            knownERC721Token.chainId = userSelectedNetworkId
          }
          if knownERC721Token.name != nameInput {
            knownERC721Token.name = nameInput
          }
          if knownERC721Token.symbol != symbolInput {
            knownERC721Token.symbol = symbolInput
          }
          token = knownERC721Token
        } else {
          token = BraveWallet.BlockchainToken(
            contractAddress: addressInput,
            name: nameInput,
            logo: "",
            isCompressed: false,
            isErc20: false,
            isErc721: network.coin != .sol && !tokenIdToHex.isEmpty,
            isErc1155: false,
            splTokenProgram: .unknown,
            isNft: true,
            isSpam: false,
            symbol: symbolInput,
            decimals: 0,
            visible: true,
            tokenId: tokenIdToHex,
            coingeckoId: coingeckoId,
            chainId: network.chainId,
            coin: network.coin
          )
        }
      }
      let success = await userAssetStore.addUserAsset(token)
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
      networkSelectionStore: .init(networkStore: .previewStore),
      keyringStore: .previewStore,
      userAssetStore: .previewStore
    )
  }
}
#endif
