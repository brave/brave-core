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
  
  var body: some View {
    _CryptoPagesView(walletStore: walletStore)
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
        AssetSearchView(
          tokenRegistry: walletStore.tokenRegistry,
          keyringStore: walletStore.keyringStore,
          networkStore: walletStore.networkStore
        )
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
    
    func makeUIViewController(context: Context) -> some UIViewController {
      CryptoPagesViewController(walletStore: walletStore)
    }
    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
    }
  }
}

private class CryptoPagesViewController: TabbedPageViewController {
  private let walletStore: WalletStore
  private let swapButton = SwapButton()
  
  init(walletStore: WalletStore) {
    self.walletStore = walletStore
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
      UIHostingController(rootView: PortfolioView(keyringStore: walletStore.keyringStore, networkStore: walletStore.networkStore, portfolioStore: walletStore.portfolioStore)).then {
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
  }
  
  @objc private func tappedSwapButton() {
    let controller = FixedHeightHostingPanModalController(
      rootView: BuySendSwapView(action: { [weak self] action in
        guard let self = self else { return }
        switch action {
        case .buy:
          break
        case .send:
          break
        case .swap:
          let controller = UIHostingController(
            rootView: SwapCryptoView(
              keyringStore: self.walletStore.keyringStore,
              ethNetworkStore: self.walletStore.networkStore
            )
          )
          self.dismiss(animated: true) {
            self.present(controller, animated: true)
          }
        }
      })
    )
    presentPanModal(
      controller,
      sourceView: swapButton,
      sourceRect: swapButton.bounds
    )
  }
}
