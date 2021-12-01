/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveCore
import SwiftUI
import BraveUI
import struct Shared.Strings

struct AccountActivityView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var activityStore: AccountActivityStore
  @ObservedObject var networkStore: NetworkStore
  
  @State private var detailsPresentation: DetailsPresentation?
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  private struct DetailsPresentation: Identifiable {
    var inEditMode: Bool
    var id: String {
      "\(inEditMode)"
    }
  }
  
  private var accountInfo: BraveWallet.AccountInfo {
    guard let info = keyringStore.keyring.accountInfos.first(where: { $0.address == activityStore.account.address }) else {
      // The account has been removed... User should technically never see this state because
      // `AccountsViewController` pops this view off the stack when the account is removed
      return activityStore.account
    }
    return info
  }
  
  private let currencyFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
  }
  
  private func emptyTextView(_ message: String) -> some View {
    Text(message)
      .font(.footnote.weight(.medium))
      .frame(maxWidth: .infinity)
      .multilineTextAlignment(.center)
      .foregroundColor(Color(.secondaryBraveLabel))
  }
  
  var body: some View {
    List {
      Section {
        AccountActivityHeaderView(account: accountInfo) { editMode in
          // Needed to use an identifiable object here instead of two @State Bool's due to a SwiftUI bug
          detailsPresentation = .init(inEditMode: editMode)
        }
        .frame(maxWidth: .infinity)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.assetsTitle))
      ) {
        if activityStore.assets.isEmpty {
          emptyTextView(Strings.Wallet.noAssets)
        } else {
          ForEach(activityStore.assets) { asset in
            PortfolioAssetView(
              image: AssetIconView(token: asset.token),
              title: asset.token.name,
              symbol: asset.token.symbol,
              amount: currencyFormatter.string(from: NSNumber(value: (Double(asset.price) ?? 0) * asset.decimalBalance)) ?? "",
              quantity: String(format: "%.04f", asset.decimalBalance)
            )
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.transactionsTitle))
      ) {
        if activityStore.transactions.isEmpty {
          emptyTextView(Strings.Wallet.noTransactions)
        } else {
          ForEach(activityStore.transactions) { tx in
            TransactionView(
              info: tx,
              keyringStore: keyringStore,
              networkStore: networkStore,
              visibleTokens: activityStore.assets.map(\.token),
              displayAccountCreator: false,
              assetRatios: activityStore.assets.reduce(into: [String: Double](), {
                $0[$1.token.symbol.lowercased()] = Double($1.price)
              })
            )
            .contextMenu {
              if !tx.txHash.isEmpty {
                Button(action: {
                  if let baseURL = self.networkStore.selectedChain.blockExplorerUrls.first.map(URL.init(string:)),
                     let url = baseURL?.appendingPathComponent("tx/\(tx.txHash)") {
                    openWalletURL?(url)
                  }
                }) {
                  Label(Strings.Wallet.viewOnBlockExplorer, systemImage: "arrow.up.forward.square")
                }
              }
            }
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .sheet(item: $detailsPresentation) {
      AccountDetailsView(
        keyringStore: keyringStore,
        account: accountInfo,
        editMode: $0.inEditMode
      )
    }
    .onReceive(keyringStore.$keyring) { keyring in
      if !keyring.accountInfos.contains(where: { $0.address == accountInfo.address }) {
        // Account was deleted
        detailsPresentation = nil
        presentationMode.dismiss()
      }
    }
    .onAppear {
      activityStore.update()
    }
  }
}

private struct AccountActivityHeaderView: View {
  var account: BraveWallet.AccountInfo
  var action: (_ tappedEdit: Bool) -> Void
  
  var body: some View {
    VStack {
      Blockie(address: account.address)
        .frame(width: 64, height: 64)
        .accessibilityHidden(true)
      VStack(spacing: 4) {
        Text(account.name)
          .fontWeight(.semibold)
        Text(account.address.truncatedAddress)
          .font(.footnote)
      }
      .padding(.bottom, 12)
      HStack {
        Button(action: { action(false) }) {
          HStack {
            Image("brave.qr-code")
              .font(.body)
            Text(Strings.Wallet.detailsButtonTitle)
              .font(.footnote.weight(.bold))
          }
        }
        Button(action: { action(true) }) {
          HStack {
            Image("brave.edit")
              .font(.body)
            Text(Strings.Wallet.renameButtonTitle)
              .font(.footnote.weight(.bold))
          }
        }
      }
      .buttonStyle(BraveOutlineButtonStyle(size: .normal))
    }
    .padding()
  }
}

#if DEBUG
struct AccountActivityView_Previews: PreviewProvider {
  static var previews: some View {
    AccountActivityView(
      keyringStore: .previewStoreWithWalletCreated,
      activityStore: .previewStore,
      networkStore: .previewStore
    )
    .previewColorSchemes()
  }
}
#endif
