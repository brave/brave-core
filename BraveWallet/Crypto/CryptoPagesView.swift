/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import PanModal
import BraveUI
import struct Shared.Strings

struct CryptoPagesView: View {
  var walletStore: WalletStore
  
  @State private var isShowingSettings: Bool = false
  @State private var isShowingSearch: Bool = false
  @State private var isShowingTransactions: Bool = false
  
  var body: some View {
    _CryptoPagesView(walletStore: walletStore, isShowingTransactions: $isShowingTransactions)
      .ignoresSafeArea()
      .navigationTitle(Strings.Wallet.cryptoTitle)
      .navigationBarTitleDisplayMode(.inline)
      .introspectViewController(customize: { vc in
        vc.navigationItem.do {
          let appearance: UINavigationBarAppearance = {
            let appearance = UINavigationBarAppearance()
            appearance.configureWithOpaqueBackground()
            appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
            appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
            appearance.backgroundColor = .braveBackground
            appearance.shadowColor = .clear
            return appearance
          }()
          $0.standardAppearance = appearance
          $0.compactAppearance = appearance
          $0.scrollEdgeAppearance = appearance
        }
      })
      .background(
        NavigationLink(
          destination: WalletSettingsView(keyringStore: walletStore.keyringStore),
          isActive: $isShowingSettings
        ) {
          Text("Settings")
        }
        .hidden()
      )
      .sheet(isPresented: $isShowingSearch) {
        AssetSearchView(walletStore: walletStore)
      }
      .sheet(isPresented: $isShowingTransactions) {
        TransactionConfirmationView(transactions: [], networkStore: walletStore.networkStore)
      }
      .toolbar {
        ToolbarItemGroup(placement: .navigationBarTrailing) {
          Button(action: {
            isShowingSearch = true
          }) {
            Image(systemName: "magnifyingglass")
              .foregroundColor(Color(.braveOrange))
          }
          Menu {
            Button(action: {
              walletStore.keyringStore.lock()
            }) {
              Label("Lock", image: "brave.lock")
                .imageScale(.medium) // Menu seems to use a different image scale by default
            }
            Divider()
            Button(action: { isShowingSettings = true }) {
              Label("Settings", image: "brave.gear")
                .imageScale(.medium) // Menu seems to use a different image scale by default
            }
          } label: {
            Image(systemName: "ellipsis.circle")
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
  }
  
  struct _CryptoPagesView: UIViewControllerRepresentable {
    var walletStore: WalletStore
    var isShowingTransactions: Binding<Bool>
    
    func makeUIViewController(context: Context) -> some UIViewController {
      CryptoPagesViewController(
        walletStore: walletStore,
        buySendSwapDestination: context.environment.buySendSwapDestination,
        isShowingTransactions: isShowingTransactions
      )
    }
    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
    }
  }
}

private class CryptoPagesViewController: TabbedPageViewController {
  private let walletStore: WalletStore
  private let swapButton = SwapButton()
  @Binding private var buySendSwapDestination: BuySendSwapDestination?
  @Binding private var isShowingTransactions: Bool
  
  init(walletStore: WalletStore, buySendSwapDestination: Binding<BuySendSwapDestination?>, isShowingTransactions: Binding<Bool>) {
    self.walletStore = walletStore
    self._buySendSwapDestination = buySendSwapDestination
    self._isShowingTransactions = isShowingTransactions
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = "Crypto"
    navigationItem.largeTitleDisplayMode = .never
    view.backgroundColor = .braveGroupedBackground
    
    pages = [
      UIHostingController(rootView: PortfolioView(
        walletStore: walletStore,
        keyringStore: walletStore.keyringStore,
        networkStore: walletStore.networkStore,
        portfolioStore: walletStore.portfolioStore
      )).then {
        $0.title = Strings.Wallet.portfolioPageTitle
      },
      UIHostingController(rootView: AccountsView(keyringStore: walletStore.keyringStore)).then {
        $0.title = Strings.Wallet.accountsPageTitle
      }
    ]
    
    view.addSubview(swapButton)
    swapButton.snp.makeConstraints {
      $0.centerX.equalToSuperview()
      $0.bottom.equalTo(view.safeAreaLayoutGuide).priority(.high)
      $0.bottom.lessThanOrEqualTo(view).inset(8)
    }
    
    pages.forEach {
      $0.additionalSafeAreaInsets = .init(top: 0, left: 0, bottom: swapButton.intrinsicContentSize.height + 8, right: 0)
    }
    
    swapButton.addTarget(self, action: #selector(tappedSwapButton), for: .touchUpInside)
    
//    let confirmationsButton = SwapButton()
//    view.addSubview(confirmationsButton)
//    confirmationsButton.snp.makeConstraints {
//      $0.trailing.equalToSuperview().inset(16)
//      $0.bottom.equalTo(view.safeAreaLayoutGuide).priority(.high)
//      $0.bottom.lessThanOrEqualTo(view).inset(8)
//    }
//    confirmationsButton.addTarget(self, action: #selector(tappedConfirmationsButton), for: .touchUpInside)
  }
  
  @objc private func tappedConfirmationsButton() {
    isShowingTransactions = true
  }
  
  @objc private func tappedSwapButton() {
    let controller = FixedHeightHostingPanModalController(
      rootView: BuySendSwapView(action: { [weak self] destination in
        self?.dismiss(animated: true, completion: {
          self?.buySendSwapDestination = destination
        })
      })
    )
    presentPanModal(
      controller,
      sourceView: swapButton,
      sourceRect: swapButton.bounds
    )
  }
}
