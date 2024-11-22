// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Combine
import Foundation
import SwiftUI

/// Methods for handling actions that occur while the user is interacting with Brave Wallet that require
/// some integration with the browser
public protocol BraveWalletDelegate: AnyObject {
  /// Open a specific URL that comes from the wallet UI. For instance, when purchasing tokens through Ramp.Network
  ///
  /// This will be called after the wallet UI is dismissed
  func openDestinationURL(_ url: URL)
  /// Requests App Review pop-up after send - swap and add account
  ///
  /// This will be called after the wallet UI is dismissed
  func requestAppReview()
  /// Present Wallet with or without dismiss the Wallet Panel depends on the value of the `presentWalletWithContext`
  func walletPanel(
    _ panel: WalletPanelHostingController,
    presentWalletWithContext: PresentingContext,
    walletStore: WalletStore
  )
}

/// The context of which wallet is being presented. Controls what content is shown when the wallet is unlocked
public enum PresentingContext {
  /// The default context shows the main wallet view which includes portfolio, buy/send/swap, etc.
  case `default`(_ selectedTab: CryptoTab)
  /// Shows the user any pending requests made by webpages such as transaction confirmations, adding networks, switch networks, add tokens, sign message, etc.
  case pendingRequests
  /// Shows when a webpage wants to connect with the users wallet
  case requestPermissions(
    _ request: WebpagePermissionRequest,
    onPermittedAccountsUpdated: (_ permittedAccounts: [String]) -> Void
  )
  /// Shows the user only the unlock/setup screen then dismisses to view an unlocked panel
  case panelUnlockOrSetup
  /// Shows the user available wallet accounts to use
  case accountSelection
  /// Shows the user transaction history of current selected account and network
  case transactionHistory
  /// Shows the user one of the three transaction action screens 1. Buy 2. Send 3. Swap 4. Deposit
  case walletAction(_ destination: WalletActionDestination)
  /// Shows the user the wallet settings screen
  case settings
  /// Shows when the users want to edit connected account the the webpage
  case editSiteConnection(_ origin: URLOrigin, handler: (_ permittedAccounts: [String]) -> Void)
  /// Shows account creation
  case createAccount(_ request: WalletProviderAccountCreationRequest)
}

/// The initial wallet controller to present when the user wants to view their wallet
public class WalletHostingViewController: UIHostingController<CryptoView> {
  public weak var delegate: BraveWalletDelegate?
  private var cancellable: AnyCancellable?
  private var walletStore: WalletStore?

  public init(
    walletStore: WalletStore,
    webImageDownloader: WebImageDownloaderType,
    presentingContext: PresentingContext = .default(.portfolio),
    onUnlock: (() -> Void)? = nil
  ) {
    gesture = WalletInteractionGestureRecognizer(
      keyringStore: walletStore.keyringStore
    )
    super.init(
      rootView: CryptoView(
        walletStore: walletStore,
        keyringStore: walletStore.keyringStore,
        webImageDownloader: webImageDownloader,
        presentingContext: presentingContext
      )
    )
    rootView.openWalletURLAction = { [unowned self] url in
      (self.presentingViewController ?? self).dismiss(animated: true) { [self] in
        self.delegate?.openDestinationURL(url)
      }
    }
    rootView.appRatingRequestAction = { [unowned self] in
      self.delegate?.requestAppReview()
    }
    cancellable = walletStore.keyringStore.$isWalletLocked
      .dropFirst()  // Drop initial value
      .removeDuplicates()
      .dropFirst()  // Drop first async fetch of keyring
      .sink { [weak self] isLocked in
        if !isLocked {
          onUnlock?()
        }
        // Prior to iOS 16.4, SwiftUI has a bug where nested sheets do not dismiss correctly if the
        // root View holding onto the sheet is removed from the view hierarchy. The root's sheet
        // stays visible even though the root doesn't exist anymore.
        //
        // As a workaround to this issue, we can just watch keyring's `isLocked` value from here
        // and dismiss the first sheet ourselves to ensure we dont get stuck with a child view visible
        // while the wallet is locked.
        if #unavailable(iOS 16.4),
          let self = self,
          isLocked,
          let presentedViewController = self.presentedViewController,
          !presentedViewController.isBeingDismissed
        {
          self.dismiss(animated: true)
        }
      }
    self.walletStore = walletStore
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  deinit {
    gesture.view?.removeGestureRecognizer(gesture)
    walletStore?.isPresentingFullWallet = false
  }

  private let gesture: WalletInteractionGestureRecognizer

  public override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    view.window?.addGestureRecognizer(gesture)

    DeviceOrientation.shared.changeOrientationToPortraitOnPhone()
    self.setNeedsUpdateOfSupportedInterfaceOrientations()
  }

  public override func viewDidLoad() {
    super.viewDidLoad()
    walletStore?.isPresentingFullWallet = true
  }

  // MARK: -

  public override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    [.portrait, .portraitUpsideDown]
  }

  public override var shouldAutorotate: Bool {
    true
  }
}

class WalletInteractionGestureRecognizer: UIGestureRecognizer {
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
