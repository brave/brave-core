// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveCore
import Combine

/// Methods for handling actions that occur while the user is interacting with Brave Wallet that require
/// some integration with the browser
public protocol BraveWalletDelegate: AnyObject {
  /// Open a specific URL that comes from the wallet UI. For instance, when purchasing tokens through Wyre
  ///
  /// This will be called after the wallet UI is dismissed
  func openWalletURL(_ url: URL)
}

/// The initial wallet controller to present when the user wants to view their wallet
public class WalletHostingViewController: UIHostingController<CryptoView> {
  public weak var delegate: BraveWalletDelegate?
  private var cancellable: AnyCancellable?
  
  public init(walletStore: WalletStore) {
    gesture = WalletInteractionGestureRecognizer(
      keyringStore: walletStore.keyringStore
    )
    super.init(
      rootView: CryptoView(
        walletStore: walletStore,
        keyringStore: walletStore.keyringStore
      )
    )
    rootView.dismissAction = { [unowned self] in
      self.dismiss(animated: true)
    }
    rootView.openWalletURLAction = { [unowned self] url in
      (presentingViewController ?? self).dismiss(animated: true) {
        self.delegate?.openWalletURL(url)
      }
    }
    // SwiftUI has a bug where nested sheets do not dismiss correctly if the root View holding onto
    // the sheet is removed from the view hierarchy. The root's sheet stays visible even though the
    // root doesn't exist anymore.
    //
    // As a workaround to this issue, we can just watch keyring's `isLocked` value from here
    // and dismiss the first sheet ourselves to ensure we dont get stuck with a child view visible
    // while the wallet is locked.
    cancellable = walletStore.keyringStore.$keyring
      .dropFirst()
      .map(\.isLocked)
      .removeDuplicates()
      .sink { [weak self] isLocked in
        guard let self = self else { return }
        if isLocked, let controller = self.presentedViewController {
          controller.dismiss(animated: true)
        }
      }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  deinit {
    view.window?.removeGestureRecognizer(gesture)
  }
  
  private let gesture: WalletInteractionGestureRecognizer
  
  public override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    view.window?.addGestureRecognizer(gesture)
  }
  
  // MARK: -
  
  public override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    [.portrait, .portraitUpsideDown]
  }
  
  public override var shouldAutorotate: Bool {
    false
  }
}

private class WalletInteractionGestureRecognizer: UIGestureRecognizer {
  private let store: KeyringStore
  init(keyringStore: KeyringStore) {
    store = keyringStore
    super.init(target: nil, action: nil)
    cancelsTouchesInView = false
    delaysTouchesEnded = false
  }
  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
    super.touchesBegan(touches, with: event)
    state = .failed
    store.notifyUserInteraction()
  }
}
