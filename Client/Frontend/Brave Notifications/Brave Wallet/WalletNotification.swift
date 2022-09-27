// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import UIKit

class WalletNotification: BraveNotification {
  struct Constant {
    static let id = "wallet-notification"
  }
  
  enum Action {
    /// The user clicked the wallet connection notification
    case connectWallet
    /// The user swiped the notification away
    case dismissed
    /// The user ignored the wallet connection notification for a given amount of time for it to automatically dismiss
    case timedOut
  }
  
  var priority: BraveNotificationPriority
  var view: UIView
  var id: String { WalletNotification.Constant.id }
  var dismissAction: (() -> Void)?
  var presentationOrigin: PresentationOrigin
  
  private let handler: (Action) -> Void
  
  func willDismiss(timedOut: Bool) {
    handler(timedOut ? .timedOut : .dismissed)
  }
  
  init(
    priority: BraveNotificationPriority,
    origin: URLOrigin,
    isUsingBottomBar: Bool,
    handler: @escaping (Action) -> Void
  ) {
    self.priority = priority
    self.presentationOrigin = isUsingBottomBar ? .bottom : .top
    self.view = WalletConnectionView(origin: origin)
    self.handler = handler
    self.setup()
  }
  
  private func setup() {
    guard let walletPanel = view as? WalletConnectionView else { return }
    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(tappedWalletConnectionView(_:)))
    walletPanel.addGestureRecognizer(tapGesture)
  }
  
  @objc private func tappedWalletConnectionView(_ sender: WalletConnectionView) {
    dismissAction?()
    handler(.connectWallet)
  }
}
