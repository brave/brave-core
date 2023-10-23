/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveUI
import Strings

struct CryptoTabsView<DismissContent: ToolbarContent>: View {
  private enum Tab: Equatable, Hashable, CaseIterable {
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
  
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  var toolbarDismissContent: DismissContent

  @State private var isShowingMainMenu: Bool = false
  @State private var isTabShowingSettings: [Tab: Bool] = Tab.allCases.reduce(into: [Tab: Bool]()) { $0[$1] = false }
  @State private var isShowingSearch: Bool = false
  @State private var fetchedPendingRequestsThisSession: Bool = false
  @State private var selectedTab: Tab = .portfolio
  
  private var isConfirmationButtonVisible: Bool {
    if case .transactions(let txs) = cryptoStore.pendingRequest {
      return !txs.isEmpty
    }
    return cryptoStore.pendingRequest != nil
  }

  var body: some View {
    TabView(selection: $selectedTab) {
      NavigationView {
        PortfolioView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore,
          networkStore: cryptoStore.networkStore,
          portfolioStore: cryptoStore.portfolioStore
        )
        .navigationTitle(Strings.Wallet.wallet)
        .navigationBarTitleDisplayMode(.inline)
        .introspectViewController(customize: { vc in
          vc.navigationItem.do {
            // no shadow when content is at top.
            let noShadowAppearance: UINavigationBarAppearance = {
              let appearance = UINavigationBarAppearance()
              appearance.configureWithOpaqueBackground()
              appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
              appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
              appearance.backgroundColor = UIColor(braveSystemName: .pageBackground)
              appearance.shadowColor = .clear
              return appearance
            }()
            $0.scrollEdgeAppearance = noShadowAppearance
            $0.compactScrollEdgeAppearance = noShadowAppearance
            // shadow when content is scrolled behind navigation bar.
            let shadowAppearance: UINavigationBarAppearance = {
              let appearance = UINavigationBarAppearance()
              appearance.configureWithOpaqueBackground()
              appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
              appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
              appearance.backgroundColor = UIColor(braveSystemName: .pageBackground)
              return appearance
            }()
            $0.standardAppearance = shadowAppearance
            $0.compactAppearance = shadowAppearance
          }
        })
        .toolbar { sharedToolbarItems }
        .background(settingsNavigationLink(for: .portfolio))
      }
      .navigationViewStyle(.stack)
      .tabItem {
        Tab.portfolio.tabLabel
      }
      .tag(Tab.portfolio)
      
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
        Tab.activity.tabLabel
      }
      .tag(Tab.activity)
      
      NavigationView {
        AccountsView(
          cryptoStore: cryptoStore,
          keyringStore: keyringStore
        )
        .navigationTitle(Strings.Wallet.wallet)
        .navigationBarTitleDisplayMode(.inline)
        .applyRegularNavigationAppearance()
        .toolbar { sharedToolbarItems }
        .background(settingsNavigationLink(for: .accounts))
      }
      .navigationViewStyle(.stack)
      .tabItem {
        Tab.accounts.tabLabel
      }
      .tag(Tab.accounts)
      
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
        Tab.market.tabLabel
      }
      .tag(Tab.market)
    }
    .introspectTabBarController(customize: { tabBarController in
      let appearance = UITabBarAppearance()
      appearance.configureWithOpaqueBackground()
      appearance.backgroundColor = UIColor(braveSystemName: .containerBackground)
      tabBarController.tabBar.standardAppearance = appearance
      tabBarController.tabBar.scrollEdgeAppearance = appearance
    })
    .overlay(alignment: .bottomTrailing, content: {
      if isConfirmationButtonVisible {
        Button(action: {
          cryptoStore.isPresentingPendingRequest = true
        }) {
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
    })
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
            userAssetsStore: cryptoStore.portfolioStore.userAssetsStore
          )
        }
    )
    .sheet(isPresented: $isShowingMainMenu) {
      MainMenuView(
        isFromPortfolio: selectedTab == .portfolio,
        isShowingSettings: Binding(get: {
          self.isTabShowingSettings[selectedTab, default: false]
        }, set: { isActive, _ in
          self.isTabShowingSettings[selectedTab] = isActive
        }),
        keyringStore: keyringStore
      )
    }
  }
  
  @ToolbarContentBuilder private var sharedToolbarItems: some ToolbarContent {
    ToolbarItemGroup(placement: .navigationBarTrailing) {
      Button(action: {
        cryptoStore.isPresentingAssetSearch = true
      }) {
        Label(Strings.Wallet.searchTitle, systemImage: "magnifyingglass")
          .labelStyle(.iconOnly)
          .foregroundColor(Color(.braveBlurpleTint))
      }
      Button(action: { self.isShowingMainMenu = true }) {
        Label(Strings.Wallet.otherWalletActionsAccessibilityTitle, braveSystemImage: "leo.more.horizontal")
          .labelStyle(.iconOnly)
          .foregroundColor(Color(.braveBlurpleTint))
      }
      .accessibilityLabel(Strings.Wallet.otherWalletActionsAccessibilityTitle)
    }
    toolbarDismissContent
  }
  
  private func settingsNavigationLink(for tab: Tab) -> some View {
    NavigationLink(
      destination: Web3SettingsView(
        settingsStore: cryptoStore.settingsStore,
        networkStore: cryptoStore.networkStore,
        keyringStore: keyringStore
      ),
      isActive: Binding(get: {
        self.isTabShowingSettings[tab, default: false]
      }, set: { isActive, _ in
        self.isTabShowingSettings[tab] = isActive
      })
    ) {
      Text(Strings.Wallet.settings)
    }
    .hidden()
  }
}

private extension View {
  func applyRegularNavigationAppearance() -> some View {
    introspectViewController(customize: { vc in
      vc.navigationItem.do {
        let appearance: UINavigationBarAppearance = {
          let appearance = UINavigationBarAppearance()
          appearance.configureWithOpaqueBackground()
          appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.backgroundColor = .braveBackground
          return appearance
        }()
        $0.standardAppearance = appearance
        $0.compactAppearance = appearance
        $0.scrollEdgeAppearance = appearance
      }
    })
  }
}
