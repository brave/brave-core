// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore
import SwiftUI
import Combine
import BraveUI

/// Displays a summary of the users wallet when they are visiting a webpage that wants to connect with the
/// users wallet
public class WalletPanelHostingController: UIHostingController<WalletPanelContainerView> {

  public weak var delegate: BraveWalletDelegate?
  private var cancellable: AnyCancellable?

  public init(
    walletStore: WalletStore,
    origin: URLOrigin,
    faviconRenderer: WalletFaviconRenderer
  ) {
    gesture = WalletInteractionGestureRecognizer(
      keyringStore: walletStore.keyringStore
    )
    super.init(rootView: WalletPanelContainerView(
      walletStore: walletStore,
      keyringStore: walletStore.keyringStore,
      origin: origin
    ))
    rootView.presentWalletWithContext = { [weak self] context in
      guard let self = self else { return }
      self.delegate?.walletPanel(self, presentWalletWithContext: context, walletStore: walletStore)
    }
    rootView.presentBuySendSwap = { [weak self] in
      guard let self = self, let store = walletStore.cryptoStore else { return }
      let controller = FixedHeightHostingPanModalController(
        rootView: BuySendSwapView(
          network: store.networkStore.selectedChain,
          action: { destination in
            self.dismiss(
              animated: true,
              completion: {
                let walletHostingController = WalletHostingViewController(walletStore: walletStore, presentingContext: .buySendSwap(destination), faviconRenderer: faviconRenderer)
                walletHostingController.delegate = self.delegate
                self.present(walletHostingController, animated: true)
              })
          })
      )
      self.presentPanModal(controller, sourceView: self.rootView.buySendSwapBackground.uiView, sourceRect: self.rootView.buySendSwapBackground.uiView.bounds)
    }
    
    // Dismiss Buy/Send/Swap Menu when Wallet becomes locked
    cancellable = walletStore.keyringStore.$defaultKeyring
      .dropFirst() // Drop initial value
      .map(\.isLocked)
      .removeDuplicates()
      .dropFirst() // Drop first async fetch of keyring
      .sink { [weak self] isLocked in
        if let self = self, isLocked, self.presentedViewController != nil {
          self.dismiss(animated: true)
        }
      }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  deinit {
    gesture.view?.removeGestureRecognizer(gesture)
  }
  
  private let gesture: WalletInteractionGestureRecognizer
  
  public override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    view.window?.addGestureRecognizer(gesture)
  }
  
  public override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    
    // For some reason these 2 calls are required in order for the `UIHostingController` to layout
    // correctly. Without this it for some reason becomes taller than what it needs to be despite its
    // `sizeThatFits(_:)` calls returning the correct value once the parent does layout.
    view.setNeedsUpdateConstraints()
    view.updateConstraintsIfNeeded()
  }
}
