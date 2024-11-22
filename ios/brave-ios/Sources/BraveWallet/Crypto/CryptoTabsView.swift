// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Strings
import SwiftUI
import UIKit

public enum CryptoTab: Equatable, Hashable, CaseIterable {
  case portfolio
  case activity
  case accounts
  case market

  @ViewBuilder var tabLabel: some View {
    switch self {
    case .portfolio:
      Label(Strings.Wallet.portfolioPageTitle, braveSystemImage: "leo.coins")
    case .activity:
      Label(Strings.Wallet.activityPageTitle, braveSystemImage: "leo.activity")
    case .accounts:
      Label(Strings.Wallet.accountsPageTitle, braveSystemImage: "leo.user.accounts")
    case .market:
      Label(Strings.Wallet.marketPageTitle, braveSystemImage: "leo.discover")
    }
  }
}

struct CryptoTabsView<DismissContent: ToolbarContent>: View {
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  var toolbarDismissContent: DismissContent

  @State private var isShowingMainMenu: Bool = false
  @State private var isTabShowingSettings: [CryptoTab: Bool] = CryptoTab.allCases.reduce(
    into: [CryptoTab: Bool]()
  ) { $0[$1] = false }
  @State private var isShowingSearch: Bool = false
  @State private var isShowingBackup: Bool = false
  @State private var isShowingAddAccount: Bool = false
  @State private var fetchedPendingRequestsThisSession: Bool = false

  @Environment(\.walletActionDestination)
  private var walletActionDestination: Binding<WalletActionDestination?>

  init(
    cryptoStore: CryptoStore,
    keyringStore: KeyringStore,
    toolbarDismissContent: DismissContent
  ) {
    self.cryptoStore = cryptoStore
    self.keyringStore = keyringStore
    self.toolbarDismissContent = toolbarDismissContent
  }

  private var isConfirmationButtonVisible: Bool {
    if case .transactions(let txs) = cryptoStore.pendingRequest {
      return !txs.isEmpty
    }
    return cryptoStore.pendingRequest != nil
  }

  var body: some View {
    TabView(selection: $cryptoStore.selectedTab) {
      NavigationView {
        PortfolioView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore,
          networkStore: cryptoStore.networkStore,
          portfolioStore: cryptoStore.portfolioStore
        )
        .navigationTitle(Strings.Wallet.wallet)
        .navigationBarTitleDisplayMode(.inline)
        .transparentUnlessScrolledNavigationAppearance()
        .toolbar { sharedToolbarItems }
        .background(settingsNavigationLink(for: .portfolio))
      }
      .navigationViewStyle(.stack)
      .tabItem {
        CryptoTab.portfolio.tabLabel
      }
      .tag(CryptoTab.portfolio)

      NavigationView {
        TransactionsActivityView(
          store: cryptoStore.transactionsActivityStore,
          networkStore: cryptoStore.networkStore
        )
        .navigationTitle(Strings.Wallet.wallet)
        .navigationBarTitleDisplayMode(.inline)
        .applyRegularNavigationAppearance()
        .toolbar { sharedToolbarItems }
        .background(settingsNavigationLink(for: .activity))
      }
      .navigationViewStyle(.stack)
      .tabItem {
        CryptoTab.activity.tabLabel
      }
      .tag(CryptoTab.activity)

      NavigationView {
        AccountsView(
          store: cryptoStore.accountsStore,
          cryptoStore: cryptoStore,
          keyringStore: keyringStore,
          walletActionDestination: walletActionDestination
        )
        .navigationTitle(Strings.Wallet.wallet)
        .navigationBarTitleDisplayMode(.inline)
        .applyRegularNavigationAppearance()
        .toolbar { sharedToolbarItems }
        .background(settingsNavigationLink(for: .accounts))
      }
      .navigationViewStyle(.stack)
      .tabItem {
        CryptoTab.accounts.tabLabel
      }
      .tag(CryptoTab.accounts)

      NavigationView {
        MarketView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore
        )
        .navigationTitle(Strings.Wallet.wallet)
        .navigationBarTitleDisplayMode(.inline)
        .applyRegularNavigationAppearance()
        .toolbar { sharedToolbarItems }
        .background(settingsNavigationLink(for: .market))
      }
      .navigationViewStyle(.stack)
      .tabItem {
        CryptoTab.market.tabLabel
      }
      .tag(CryptoTab.market)
    }
    .introspectTabBarController(customize: { tabBarController in
      let appearance = UITabBarAppearance()
      appearance.configureWithOpaqueBackground()
      appearance.backgroundColor = UIColor(braveSystemName: .containerBackground)
      tabBarController.tabBar.standardAppearance = appearance
      tabBarController.tabBar.scrollEdgeAppearance = appearance
    })
    .overlay(
      alignment: .bottomTrailing,
      content: {
        if isConfirmationButtonVisible {
          Button {
            cryptoStore.isPresentingPendingRequest = true
          } label: {
            Image(braveSystemName: "leo.notification.dot")
              .font(.system(size: 18))
              .foregroundColor(.white)
              .frame(width: 36, height: 36)
              .background(
                Color(uiColor: .braveBlurpleTint)
                  .clipShape(Circle())
              )
          }
          .accessibilityLabel(Text(Strings.Wallet.confirmTransactionsTitle))
          .padding(.trailing, 16)
          .padding(.bottom, 100)
        }
      }
    )
    .onAppear {
      // If a user chooses not to confirm/reject their requests we shouldn't
      // do it again until they close and re-open wallet
      if !fetchedPendingRequestsThisSession {
        // Give the animation time
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
          self.fetchedPendingRequestsThisSession = true
          self.cryptoStore.prepare(isInitialOpen: true)
        }
      }
    }
    .ignoresSafeArea()
    .background(
      Color.clear
        .sheet(isPresented: $cryptoStore.isPresentingAssetSearch) {
          AssetSearchView(
            keyringStore: keyringStore,
            cryptoStore: cryptoStore,
            userAssetsStore: cryptoStore.openUserAssetsStore()
          )
          .onDisappear {
            cryptoStore.closeUserAssetsStore()
          }
        }
    )
    .sheet(isPresented: $isShowingMainMenu) {
      MainMenuView(
        selectedTab: cryptoStore.selectedTab,
        isShowingSettings: Binding(
          get: {
            self.isTabShowingSettings[cryptoStore.selectedTab, default: false]
          },
          set: { isActive, _ in
            self.isTabShowingSettings[cryptoStore.selectedTab] = isActive
          }
        ),
        isShowingBackup: $isShowingBackup,
        isShowingAddAccount: $isShowingAddAccount,
        keyringStore: keyringStore
      )
      .background(
        Color.clear
          .sheet(
            isPresented: Binding(
              get: {
                isShowingBackup
              },
              set: { newValue in
                if !newValue {
                  // dismiss menu if we're dismissing backup from menu
                  isShowingMainMenu = false
                }
                isShowingBackup = newValue
              }
            )
          ) {
            NavigationView {
              BackupWalletView(
                password: nil,
                keyringStore: keyringStore
              )
            }
            .navigationViewStyle(.stack)
            .environment(\.modalPresentationMode, $isShowingBackup)
            .accentColor(Color(.braveBlurpleTint))
          }
      )
      .background(
        Color.clear
          .sheet(
            isPresented: Binding(
              get: {
                isShowingAddAccount
              },
              set: { newValue in
                if !newValue {
                  // dismiss menu if we're dismissing add account from menu
                  isShowingMainMenu = false
                }
                isShowingAddAccount = newValue
              }
            )
          ) {
            NavigationView {
              AddAccountView(
                keyringStore: keyringStore,
                networkStore: cryptoStore.networkStore
              )
            }
            .navigationViewStyle(StackNavigationViewStyle())
          }
      )
    }
  }

  @ToolbarContentBuilder private var sharedToolbarItems: some ToolbarContent {
    ToolbarItemGroup(placement: .navigationBarTrailing) {
      if cryptoStore.selectedTab == .portfolio {
        Button {
          cryptoStore.isPresentingAssetSearch = true
        } label: {
          Label(Strings.Wallet.searchTitle, systemImage: "magnifyingglass")
            .labelStyle(.iconOnly)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      Button {
        self.isShowingMainMenu = true
      } label: {
        Label(
          Strings.Wallet.otherWalletActionsAccessibilityTitle,
          braveSystemImage: "leo.more.horizontal"
        )
        .labelStyle(.iconOnly)
        .foregroundColor(Color(.braveBlurpleTint))
      }
      .accessibilityLabel(Strings.Wallet.otherWalletActionsAccessibilityTitle)
    }
    toolbarDismissContent
  }

  private func settingsNavigationLink(for tab: CryptoTab) -> some View {
    NavigationLink(
      destination: Web3SettingsView(
        settingsStore: cryptoStore.settingsStore,
        networkStore: cryptoStore.networkStore,
        keyringStore: keyringStore
      ),
      isActive: Binding(
        get: {
          self.isTabShowingSettings[tab, default: false]
        },
        set: { isActive, _ in
          self.isTabShowingSettings[tab] = isActive
        }
      )
    ) {
      Text(Strings.Wallet.settings)
    }
    .hidden()
  }
}
