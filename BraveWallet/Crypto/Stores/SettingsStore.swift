// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

public class SettingsStore: ObservableObject {
  /// The number of minutes to wait until the Brave Wallet is automatically locked
  @Published var autoLockInterval: AutoLockInterval = .minute {
    didSet {
      keyringService.setAutoLockMinutes(autoLockInterval.value) { _ in }
    }
  }
  
  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  
  public init(
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    
    self.keyringService.autoLockMinutes { minutes in
      self.autoLockInterval = .init(value: minutes)
    }
  }
  
  func reset() {
    walletService.reset()
    KeyringStore.resetKeychainStoredPassword()
  }
  
  public func isDefaultKeyringCreated(_ completion: @escaping (Bool) -> Void) {
    keyringService.defaultKeyringInfo { keyring in
      completion(keyring.isDefaultKeyringCreated)
    }
  }
  
  public func addKeyringServiceObserver(_ observer: BraveWalletKeyringServiceObserver) {
    keyringService.add(observer)
  }
}
