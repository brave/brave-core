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

class CryptoPagesViewController: TabbedPageViewController {
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
    
    navigationItem.leftBarButtonItem = .init(
      image: UIImage(imageLiteralResourceName: "wallet-dismiss"),
      style: .plain,
      target: self,
      action: #selector(closeWallet)
    )
    navigationItem.rightBarButtonItem = .init(
      image: UIImage(imageLiteralResourceName: "wallet-search"),
      style: .plain,
      target: self,
      action: #selector(presentSearch)
    )
    
    navigationItem.do {
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
  
  @objc private func presentSearch() {
    let controller = UIHostingController(
      rootView: AssetSearchView(
        tokenRegistry: BraveCoreMain.ercTokenRegistry,
        keyringStore: walletStore.keyringStore,
        networkStore: walletStore.networkStore
      )
    )
    present(controller, animated: true)
  }
  
  @objc private func closeWallet() {
    dismiss(animated: true)
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
