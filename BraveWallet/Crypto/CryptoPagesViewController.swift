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

@available(iOS 14.0, *)
class CryptoPagesViewController: TabbedPageViewController {
  private let keyringStore: KeyringStore
  private let networkStore: EthNetworkStore
  private let swapButton = SwapButton()
  
  init(
    keyringStore: KeyringStore,
    networkStore: EthNetworkStore
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
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
      UIHostingController(rootView: PortfolioView(keyringStore: keyringStore, networkStore: networkStore)).then {
        $0.title = "Portfolio" // NSLocalizedString
      },
      UIHostingController(rootView: AccountsView(keyringStore: keyringStore)).then {
        $0.title = "Accounts" // NSLocalizedString
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
    let controller = UIHostingController(rootView: AssetSearchView(tokenRegistry: TestTokenRegistry()))
    present(controller, animated: true)
  }
  
  @objc private func closeWallet() {
    dismiss(animated: true)
  }
  
  @objc private func tappedSwapButton() {
    let controller = FixedHeightHostingPanModalController(
      rootView: BuySendSwapView(action: { action in
      
      })
    )
    presentPanModal(
      controller,
      sourceView: swapButton,
      sourceRect: swapButton.bounds
    )
  }
}
