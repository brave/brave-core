// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Preferences
import Strings
import SwiftUI

struct AddAccountView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  @State private var name: String = ""
  @State private var privateKey: String = ""
  @State private var isPresentingImport: Bool = false
  @State private var isLoadingFile: Bool = false
  @State private var originPassword: String = ""
  @State private var failedToImport: Bool = false
  @State private var selectedCoin: BraveWallet.CoinType?
  @State private var accountNetwork: BraveWallet.NetworkInfo
  @State private var isCreatingAccount: Bool = false
  @ScaledMetric(relativeTo: .body) private var privateKeyFieldHeight: CGFloat = 140.0
  @Environment(\.presentationMode) @Binding var presentationMode
  @Environment(\.appRatingRequestAction) private var appRatingRequest

  @ScaledMetric private var iconSize = 40.0
  private let maxIconSize: CGFloat = 80.0

  var preSelectedCoin: BraveWallet.CoinType?
  var preSelectedAccountNetwork: BraveWallet.NetworkInfo?
  var onCreate: (() -> Void)?
  var onDismiss: (() -> Void)?

  private var selectedCoinNetworks: [BraveWallet.NetworkInfo] {
    guard let coin: BraveWallet.CoinType = preSelectedCoin ?? selectedCoin else { return [] }
    return networkStore.allChains.filter { $0.coin == coin }
  }

  init(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    preSelectedCoin: BraveWallet.CoinType? = nil,
    preSelectedAccountNetwork: BraveWallet.NetworkInfo? = nil,
    onCreate: (() -> Void)? = nil,
    onDismiss: (() -> Void)? = nil
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.preSelectedCoin = preSelectedCoin
    self.onCreate = onCreate
    self.onDismiss = onDismiss
    // make sure the prefilled
    if let preSelectedAccountNetwork {
      // network is a Filecoin network
      self.preSelectedAccountNetwork = preSelectedAccountNetwork
      _accountNetwork = .init(initialValue: preSelectedAccountNetwork)
    } else {
      let networks = networkStore.allChains.filter { $0.coin == preSelectedCoin }
      _accountNetwork = .init(initialValue: networks.first ?? .init())
    }
  }

  private func addAccount(for coin: BraveWallet.CoinType) {
    if !name.isEmpty && !name.isValidAccountName {
      // User entered name is invalid
      return
    }  // else empty, KeyringStore will assign default account name

    Task { @MainActor in
      self.isCreatingAccount = true
      defer { self.isCreatingAccount = false }
      if privateKey.isEmpty {
        // Add normal account
        let success = await keyringStore.addPrimaryAccount(
          name,
          coin: coin,
          chainId: accountNetwork.chainId
        )
        if success {
          onCreate?()
          appRatingRequest?()
          presentationMode.dismiss()
        }
      } else {
        let accountInfo: BraveWallet.AccountInfo?
        if isJSONImported {
          accountInfo = await keyringStore.addSecondaryAccount(
            name,
            coin: coin,
            chainId: accountNetwork.chainId,
            json: privateKey,
            password: originPassword
          )
        } else {
          accountInfo = await keyringStore.addSecondaryAccount(
            name,
            coin: coin,
            chainId: accountNetwork.chainId,
            privateKey: privateKey
          )
        }
        if accountInfo != nil {
          onCreate?()
          appRatingRequest?()
          presentationMode.dismiss()
        } else {
          failedToImport = true
        }
      }
    }
  }

  private var isJSONImported: Bool {
    guard selectedCoin == .eth, let data = privateKey.data(using: .utf8) else {
      return false
    }
    do {
      _ = try JSONSerialization.jsonObject(with: data, options: [])
      return true
    } catch {
      return false
    }
  }

  private var showCoinSelection: Bool {
    preSelectedCoin == nil && WalletConstants.supportedCoinTypes().count > 1
  }

  private var navigationTitle: String {
    if preSelectedCoin == nil, selectedCoin == nil {
      return Strings.Wallet.addAccountTitle
    }
    return String.localizedStringWithFormat(
      Strings.Wallet.addAccountWithCoinTypeTitle,
      preSelectedCoin?.localizedTitle
        ?? (selectedCoin?.localizedTitle ?? BraveWallet.CoinType.eth.localizedTitle)
    )
  }

  @ViewBuilder private var addAccountView: some View {
    List {
      if isKeyringSelectionRequired {
        Menu(
          content: {
            Picker(selection: $accountNetwork) {
              ForEach(selectedCoinNetworks) { network in
                Text(network.chainName)
                  .foregroundColor(Color(.secondaryBraveLabel))
                  .tag(network)
              }
            } label: {
              EmptyView()
            }
          },
          label: {
            HStack {
              Text(Strings.Wallet.transactionDetailsNetworkTitle)
                .foregroundColor(Color(.braveLabel))
              Spacer()
              Group {
                Text(accountNetwork.chainName)
                Image(systemName: "chevron.up.chevron.down")
              }
              .foregroundColor(Color(.secondaryBraveLabel))
            }
          }
        )
        .disabled(preSelectedAccountNetwork != nil)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      accountNameSection
      if isJSONImported {
        originPasswordSection
      }
      privateKeySection
    }
    .listStyle(.insetGrouped)
    .scrollContentBackground(.hidden)
    .background(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(navigationTitle)
    .navigationBarItems(
      // Have to use this instead of toolbar placement to have a custom button style
      trailing: Button {
        addAccount(for: preSelectedCoin ?? (selectedCoin ?? .eth))
      } label: {
        ZStack {  // maintain button size
          ProgressView()
            .progressViewStyle(.braveCircular(size: .small))
            .hidden(isHidden: !isCreatingAccount)
          Text(Strings.Wallet.add)
            .hidden(isHidden: isCreatingAccount)
        }
      }
      .buttonStyle(BraveFilledButtonStyle(size: .small))
      .disabled(!name.isValidAccountName || isCreatingAccount)
    )
    .alert(isPresented: $failedToImport) {
      Alert(
        title: Text(Strings.Wallet.failedToImportAccountErrorTitle),
        message: Text(Strings.Wallet.failedToImportAccountErrorMessage),
        dismissButton: .cancel(Text(Strings.OKString))
      )
    }
  }

  @ViewBuilder private var coinSelectionView: some View {
    List {
      Section(
        header: WalletListHeaderView(
          title: Text(Strings.Wallet.coinTypeSelectionHeader)
        )
      ) {
        ForEach(WalletConstants.supportedCoinTypes().elements) { coin in
          NavigationLink(
            tag: coin,
            selection: $selectedCoin
          ) {
            addAccountView
              .onDisappear {
                name = ""
                originPassword = ""
                privateKey = ""
              }
          } label: {
            HStack(spacing: 10) {
              Image(coin.iconName, bundle: .module)
                .resizable()
                .aspectRatio(contentMode: .fit)
                .clipShape(Circle())
                .frame(width: min(iconSize, maxIconSize), height: min(iconSize, maxIconSize))
              VStack(alignment: .leading, spacing: 3) {
                Text(coin.localizedTitle)
                  .foregroundColor(Color(.bravePrimary))
                  .font(.headline)
                  .multilineTextAlignment(.leading)
                Text(coin.localizedDescription)
                  .foregroundColor(Color(.braveLabel))
                  .font(.footnote)
                  .multilineTextAlignment(.leading)
              }
            }
            .padding(.vertical, 10)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
    }
    .listStyle(.insetGrouped)
    .scrollContentBackground(.hidden)
    .background(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Strings.Wallet.addAccountTitle)
  }

  var body: some View {
    Group {
      if showCoinSelection {
        coinSelectionView
      } else {
        addAccountView
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button {
          onDismiss?()
          presentationMode.dismiss()
        } label: {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .onChange(of: selectedCoin) { coin in
      if coin == .fil || coin == .btc {
        accountNetwork = selectedCoinNetworks.first(where: { $0.coin == coin }) ?? .init()
      }
    }
    .sheet(isPresented: $isPresentingImport) {
      DocumentOpenerView(allowedContentTypes: [.text, .json]) { urls in
        guard let fileURL = urls.first else { return }
        self.isLoadingFile = true
        DispatchQueue.global(qos: .userInitiated).async {
          do {
            let data = try String(contentsOf: fileURL)
            DispatchQueue.main.async {
              self.privateKey = data
              self.isLoadingFile = false
            }
          } catch {
            DispatchQueue.main.async {
              // Error: Couldn't load file
              self.isLoadingFile = false
            }
          }
        }
      }
    }
    .animation(.default, value: isJSONImported)
  }

  private var accountNameSection: some View {
    Section(
      content: {
        TextField(Strings.Wallet.accountDetailsNamePlaceholder, text: $name, axis: .vertical)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      },
      header: {
        WalletListHeaderView(
          title: Text(Strings.Wallet.accountDetailsNameTitle)
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.bravePrimary))
        )
      },
      footer: {
        SectionFooterErrorView(
          errorMessage: name.isValidAccountName
            ? nil
            : String.localizedStringWithFormat(
              Strings.Wallet.accountNameLengthError,
              BraveWallet.AccountNameMaxCharacterLength
            )
        )
      }
    )
  }

  private var originPasswordSection: some View {
    Section(
      header: WalletListHeaderView(
        title: Text(Strings.Wallet.importAccountOriginPasswordTitle)
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
      )
    ) {
      SecureField(Strings.Wallet.passwordPlaceholder, text: $originPassword)
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }

  private var isJsonImportSupported: Bool {
    // nil is possible if Solana is disabled
    selectedCoin == nil || selectedCoin == .eth
  }

  @ViewBuilder private var privateKeySection: some View {
    Section(
      header: WalletListHeaderView(
        title: Text(Strings.Wallet.importAccountSectionTitle)
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
      )
    ) {
      if self.selectedCoin == .btc {
        HStack {
          Image(braveSystemName: "leo.warning.triangle-filled")
            .foregroundColor(Color(braveSystemName: .systemfeedbackWarningIcon))
          Text(
            String.localizedStringWithFormat(
              Strings.Wallet.bitcoinImportExtendedKeyWarning,
              accountNetwork.chainId == BraveWallet.BitcoinMainnet ? "zprv" : "vprv"
            )
          )
          .font(.caption)
          .foregroundColor(Color(braveSystemName: .systemfeedbackWarningText))
        }
        .listRowBackground(Color(braveSystemName: .systemfeedbackWarningBackground))
      }
      Group {
        TextEditor(text: $privateKey)
          .autocapitalization(.none)
          .font(.system(.body, design: .monospaced))
          .frame(height: privateKeyFieldHeight)
          .background(
            Text(
              isJsonImportSupported
                ? Strings.Wallet.importAccountPlaceholder
                : Strings.Wallet.importNonEthAccountPlaceholder
            )
            .padding(.vertical, 8)
            .padding(.horizontal, 4)  // To match the TextEditor's editing insets
            .frame(maxWidth: .infinity, alignment: .leading)
            .foregroundColor(Color(.placeholderText))
            .opacity(privateKey.isEmpty ? 1 : 0)
            .accessibilityHidden(true),
            alignment: .top
          )
          .introspectTextView { textView in
            textView.smartQuotesType = .no
          }
          .accessibilityValue(
            privateKey.isEmpty ? Strings.Wallet.importAccountPlaceholder : privateKey
          )
        if isJsonImportSupported {
          Button {
            isPresentingImport = true
          } label: {
            HStack {
              Text(Strings.Wallet.importButtonTitle)
                .foregroundColor(.accentColor)
                .font(.callout)
              if isLoadingFile {
                ProgressView()
                  .progressViewStyle(CircularProgressViewStyle())
              }
            }
            .frame(maxWidth: .infinity)
          }
          .disabled(isLoadingFile)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listRowSeparator(.hidden)
  }

  private var isKeyringSelectionRequired: Bool {
    guard !selectedCoinNetworks.isEmpty else { return false }
    if selectedCoin == .fil || preSelectedCoin == .fil {
      return true
    } else if (selectedCoin == .btc || preSelectedCoin == .btc)
      && Preferences.Wallet.isBitcoinTestnetEnabled.value
    {
      return true
    }
    return false
  }
}

#if DEBUG
struct AddAccountView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AddAccountView(
        keyringStore: .previewStore,
        networkStore: .previewStore
      )
    }
  }
}
#endif
