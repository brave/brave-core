/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveCore
import Combine
import SwiftUI
import BraveUI
import Strings

struct AccountsView: View {
  @ObservedObject var store: AccountsStore
  var cryptoStore: CryptoStore
  var keyringStore: KeyringStore
  /// When populated, account activity pushed for given account (assets, transactions)
  @State private var selectedAccountActivity: BraveWallet.AccountInfo?
  /// When populated, account info presented modally for given account (rename, export private key)
  @State private var selectedAccountForEdit: BraveWallet.AccountInfo?
  
  @State private var selectedAccountForExport: BraveWallet.AccountInfo?
  
  var body: some View {
    ScrollView {
      LazyVStack(spacing: 16) {
        Section(content: {
          ForEach(store.primaryAccounts) { accountDetails in
            AccountCardView(
              account: accountDetails.account,
              tokensWithBalances: accountDetails.tokensWithBalance,
              balance: accountDetails.totalBalanceFiat,
              isLoading: store.isLoading,
              action: { action in
                handle(action: action, for: accountDetails.account)
              }
            )
          }
        })
        
        if !store.importedAccounts.isEmpty {
          Section(content: {
            ForEach(store.importedAccounts) { accountDetails in
              AccountCardView(
                account: accountDetails.account,
                tokensWithBalances: accountDetails.tokensWithBalance,
                balance: accountDetails.totalBalanceFiat,
                isLoading: store.isLoading,
                action: { action in
                  handle(action: action, for: accountDetails.account)
                }
              )
            }
          }, header: {
            Text(Strings.Wallet.importedCryptoAccountsTitle)
              .font(.headline.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .textPrimary))
              .frame(maxWidth: .infinity, alignment: .leading)
          })
        }
      }
      .padding(16)
    }
    .navigationTitle(Strings.Wallet.accountsPageTitle)
    .navigationBarTitleDisplayMode(.inline)
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedAccountActivity != nil },
          set: { if !$0 { selectedAccountActivity = nil } }
        ),
        destination: {
          if let account = selectedAccountActivity {
            AccountActivityView(
              keyringStore: keyringStore,
              activityStore: cryptoStore.accountActivityStore(
                for: account,
                observeAccountUpdates: false
              ),
              networkStore: cryptoStore.networkStore
            )
            .onDisappear {
              cryptoStore.closeAccountActivityStore(for: account)
            }
          }
        },
        label: {
          EmptyView()
        })
    )
    .background(
      Color.clear
        .sheet(isPresented: Binding(
          get: { selectedAccountForEdit != nil },
          set: { if !$0 { selectedAccountForEdit = nil } }
        )) {
          if let account = selectedAccountForEdit {
            AccountDetailsView(
              keyringStore: keyringStore,
              account: account,
              editMode: true
            )
          }
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: Binding(
          get: { selectedAccountForExport != nil },
          set: { if !$0 { selectedAccountForExport = nil } }
        )) {
          if let account = selectedAccountForExport {
            NavigationView {
              AccountPrivateKeyView(
                keyringStore: keyringStore,
                account: account
              )
              .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                  Button(action: {
                    selectedAccountForExport = nil
                  }) {
                    Text(Strings.cancelButtonTitle)
                      .foregroundColor(Color(.braveBlurpleTint))
                  }
                }
              }
            }
          }
        }
    )
    .onAppear {
      store.update()
    }
  }
  
  private func handle(action: AccountCardView.Action, for account: BraveWallet.AccountInfo) {
    switch action {
    case .viewDetails:
      selectedAccountActivity = account
    case .editDetails:
      selectedAccountForEdit = account
    case .viewOnBlockExplorer:
      break
    case .exportAccount:
      selectedAccountForExport = account
    case .depositToAccount:
      break
    }
  }
}

#if DEBUG
struct AccountsViewController_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      AccountsView(
        store: .previewStore,
        cryptoStore: .previewStore,
        keyringStore: .previewStoreWithWalletCreated
      )
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif

private struct AccountCardView: View {
  
  enum Action: Equatable {
    case viewDetails
    case editDetails
    case viewOnBlockExplorer
    case exportAccount
    case depositToAccount
  }
  
  let account: BraveWallet.AccountInfo
  let tokensWithBalances: [BraveWallet.BlockchainToken]
  let balance: String
  let isLoading: Bool
  let action: (Action) -> Void
  
  @Environment(\.colorScheme) private var colorScheme: ColorScheme
  @ScaledMetric private var avatarSize = 40.0
  private let maxAvatarSize: CGFloat = 80.0
  private let contentPadding: CGFloat = 16
  
  /// Content for the top section of the card. Buttons may be hidden so we can overlay
  /// the buttons on the card itself to not interfere with touches on the card itself.
  private func topSectionContent(hidingButtons: Bool = true) -> some View {
    HStack {
      HStack(spacing: 8) {
        Blockie(address: account.address)
          .frame(width: min(avatarSize, maxAvatarSize), height: min(avatarSize, maxAvatarSize))
        VStack(alignment: .leading) {
          AddressView(address: account.address) {
            // VStack keeps views together when showing context menu w/ address
            VStack(alignment: .leading) {
              Text(account.name)
                .font(.headline.weight(.semibold))
                .foregroundColor(Color(braveSystemName: .textPrimary))
              Text(account.address.truncatedAddress)
                .font(.footnote)
            }
          }
          Text(account.accountSupportDisplayString)
            .font(.footnote)
        }
        .foregroundColor(Color(braveSystemName: .textSecondary))
      }
      .hidden(isHidden: !hidingButtons)
      Spacer()
      
      // buttons can be hidden so it's used in layout, but displayed
      // in overlay to not interfere with row button touches.
      buttons
        .hidden(isHidden: hidingButtons)
    }
    .padding(contentPadding)
  }
  
  private var buttons: some View {
    HStack(spacing: 12) {
      /*
       TODO: Accounts Block Explorer #8638
      Button(action: {
        action(.viewOnBlockExplorer)
      }) {
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
          .frame(width: 36, height: 36)
          .overlay {
            Image(braveSystemName: "leo.web3.blockexplorer")
              .foregroundColor(Color(braveSystemName: .iconInteractive))
          }
      }
      */
      Menu {
        Button(action: {
          action(.viewDetails)
        }) {
          Label(Strings.Wallet.viewDetails, braveSystemImage: "leo.eye.on")
        }
        Button(action: {
          action(.editDetails)
        }) {
          Label(Strings.Wallet.editButtonTitle, braveSystemImage: "leo.edit.pencil")
        }
        Divider()
        Button(action: {
          action(.exportAccount)
        }) {
          Label(Strings.Wallet.exportButtonTitle, braveSystemImage: "leo.key")
        }
        /*
         TODO: Account Deposit UI #8639
        Button(action: {
          action(.depositToAccount)
        }) {
          Label("Deposit", braveSystemImage: "leo.qr.code")
        }
        */
      } label: {
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
          .frame(width: 36, height: 36)
          .overlay {
            Image(braveSystemName: "leo.more.horizontal")
              .foregroundColor(Color(braveSystemName: .iconInteractive))
          }
      }
    }
  }
  
  private var bottomSectionContent: some View {
    HStack {
      if isLoading && tokensWithBalances.isEmpty {
        RoundedRectangle(cornerRadius: 4)
          .fill(Color(white: 0.9))
          .frame(width: 48, height: 24)
          .redacted(reason: .placeholder)
          .shimmer(true)
        Spacer()
        Text("$0.00")
          .font(.title3.weight(.medium))
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .redacted(reason: .placeholder)
          .shimmer(true)
      } else {
        MultipleAssetIconsView(
          tokens: tokensWithBalances,
          iconSize: 24,
          maxIconSize: 32
        )
        Spacer()
        Text(balance)
          .font(.title3.weight(.medium))
          .foregroundColor(Color(braveSystemName: .textPrimary))
      }
    }
    .padding(contentPadding)
  }
  
  var body: some View {
    Button(action: {
      action(.viewDetails)
    }) {
      VStack(spacing: 0) {
        topSectionContent()
          .background(colorScheme == .dark ? Color.black.opacity(0.5) : Color.white.opacity(0.5))
        
        bottomSectionContent
          .background(
            colorScheme == .dark ? Color.black.opacity(0.4) : Color.clear
          )
      }
      .background(
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
          .background(cardBackground)
      )
      .clipShape(RoundedRectangle(cornerRadius: 8))
      .frame(maxWidth: .infinity)
    }
    .buttonStyle(.plain)
    .overlay(alignment: .top) {
      topSectionContent(hidingButtons: false)
    }
  }
  
  private var cardBackground: some View {
    BlockieMaterial(address: account.address)
      .blur(radius: 25, opaque: true)
      .opacity(0.3)
      .clipShape(RoundedRectangle(cornerRadius: 8))
  }
}
