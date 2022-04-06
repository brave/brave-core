// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class WalletNotification: BraveNotification {
  private struct Constant {
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
  
  private let handler: (Action) -> Void
  
  func willDismiss(timedOut: Bool) {
    handler(timedOut ? .timedOut : .dismissed)
  }
  
  init(
    priority: BraveNotificationPriority,
    handler: @escaping (Action) -> Void
  ) {
    self.priority = priority
    self.view = WalletConnectionView()
    self.handler = handler
    self.setup()
  }
  
  private func setup() {
    guard let walletPanel = view as? WalletConnectionView else { return }
    walletPanel.addTarget(self, action: #selector(tappedWalletConnectionView(_:)), for: .touchUpInside)
  }
  
  @objc private func tappedWalletConnectionView(_ sender: WalletConnectionView) {
    dismissAction?()
    handler(.connectWallet)
  }
}
