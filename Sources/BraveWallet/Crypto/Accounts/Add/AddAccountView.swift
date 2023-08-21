/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveUI
import DesignSystem
import Strings
import BraveCore

struct AddAccountView: View {
  @ObservedObject var keyringStore: KeyringStore

  @State private var name: String = ""
  @State private var privateKey: String = ""
  @State private var isPresentingImport: Bool = false
  @State private var isLoadingFile: Bool = false
  @State private var originPassword: String = ""
  @State private var failedToImport: Bool = false
  @State private var selectedCoin: BraveWallet.CoinType?
  @ScaledMetric(relativeTo: .body) private var privateKeyFieldHeight: CGFloat = 140.0
  @Environment(\.presentationMode) @Binding var presentationMode
  @Environment(\.appRatingRequestAction) private var appRatingRequest

  @ScaledMetric private var iconSize = 40.0
  private let maxIconSize: CGFloat = 80.0
  
  var preSelectedCoin: BraveWallet.CoinType?
  var onCreate: (() -> Void)?
  var onDismiss: (() -> Void)?

  private func addAccount(for coin: BraveWallet.CoinType) {
    let accountName = name.isEmpty ? defaultAccountName(for: coin, isPrimary: privateKey.isEmpty) : name
    guard accountName.isValidAccountName else { return }
    
    if privateKey.isEmpty {
      // Add normal account
      keyringStore.addPrimaryAccount(accountName, coin: coin) { success in
        if success {
          onCreate?()
          appRatingRequest?()
          presentationMode.dismiss()
        }
      }
    } else {
      let handler: (BraveWallet.AccountInfo?) -> Void = { accountInfo in
        if accountInfo != nil {
          onCreate?()
          appRatingRequest?()
          presentationMode.dismiss()
        } else {
          failedToImport = true
        }
      }
      if isJSONImported {
        keyringStore.addSecondaryAccount(accountName, json: privateKey, password: originPassword, completion: handler)
      } else {
        keyringStore.addSecondaryAccount(accountName, coin: coin, privateKey: privateKey, completion: handler)
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
    preSelectedCoin == nil && WalletConstants.supportedCoinTypes.count > 1
  }
  
  private var navigationTitle: String {
    if preSelectedCoin == nil, selectedCoin == nil {
      return Strings.Wallet.addAccountTitle
    }
    return String.localizedStringWithFormat(Strings.Wallet.addAccountWithCoinTypeTitle, preSelectedCoin?.localizedTitle ?? (selectedCoin?.localizedTitle ?? BraveWallet.CoinType.eth.localizedTitle))
  }
  
  @ViewBuilder private var addAccountView: some View {
    List {
      accountNameSection
      if isJSONImported {
        originPasswordSection
      }
      privateKeySection
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(navigationTitle)
    .navigationBarItems(
      // Have to use this instead of toolbar placement to have a custom button style
      trailing: Button(action: {
        addAccount(for: preSelectedCoin ?? (selectedCoin ?? .eth))
      }) {
        Text(Strings.Wallet.add)
      }
        .buttonStyle(BraveFilledButtonStyle(size: .small))
        .disabled(!name.isValidAccountName)
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
        ForEach(Array(WalletConstants.supportedCoinTypes)) { coin in
          NavigationLink(
            tag: coin,
            selection: $selectedCoin) {
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
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
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
        Button(action: {
          onDismiss?()
          presentationMode.dismiss()
        }) {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
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
        Group {
          if #available(iOS 16, *) {
            TextField(Strings.Wallet.accountDetailsNamePlaceholder, text: $name, axis: .vertical)
          } else {
            TextField(Strings.Wallet.accountDetailsNamePlaceholder, text: $name)
          }
        }
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
          errorMessage: name.isValidAccountName ? nil : Strings.Wallet.accountNameLengthError
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

  private var privateKeySection: some View {
    Section(
      header: WalletListHeaderView(
        title: Text(Strings.Wallet.importAccountSectionTitle)
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
      )
    ) {
      Group {
        TextEditor(text: $privateKey)
          .autocapitalization(.none)
          .font(.system(.body, design: .monospaced))
          .frame(height: privateKeyFieldHeight)
          .background(
            Text(isJsonImportSupported ? Strings.Wallet.importAccountPlaceholder : Strings.Wallet.importNonEthAccountPlaceholder)
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
          .accessibilityValue(privateKey.isEmpty ? Strings.Wallet.importAccountPlaceholder : privateKey)
        if isJsonImportSupported {
          Button(action: { isPresentingImport = true }) {
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
  }
  
  private func defaultAccountName(for coin: BraveWallet.CoinType, isPrimary: Bool) -> String {
    let keyringInfo = keyringStore.allKeyrings.first { $0.coin == coin }
    if isPrimary {
      return String.localizedStringWithFormat(coin.defaultAccountName, (keyringInfo?.accountInfos.filter(\.isPrimary).count ?? 0) + 1)
    } else {
      return String.localizedStringWithFormat(coin.defaultSecondaryAccountName, (keyringInfo?.accountInfos.filter(\.isImported).count ?? 0) + 1)
    }
  }
}

#if DEBUG
struct AddAccountView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AddAccountView(keyringStore: .previewStore)
    }
  }
}
#endif
