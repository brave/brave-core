// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import LocalAuthentication
import BraveCore
import Data

public class SettingsStore: ObservableObject {
  /// The number of minutes to wait until the Brave Wallet is automatically locked
  @Published var autoLockInterval: AutoLockInterval = .minute {
    didSet {
      keyringService.setAutoLockMinutes(autoLockInterval.value) { _ in }
    }
  }

  /// If we should attempt to unlock via biometrics (Face ID / Touch ID)
  var isBiometricsUnlockEnabled: Bool {
    KeyringStore.isKeychainPasswordStored && isBiometricsAvailable
  }

  /// If the device has biometrics available
  var isBiometricsAvailable: Bool {
    LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  }

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletTxService

  public init(
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    self.txService = txService

    self.keyringService.autoLockMinutes { minutes in
      self.autoLockInterval = .init(value: minutes)
    }
  }

  func reset() {
    walletService.reset()
    KeyringStore.resetKeychainStoredPassword()
    Domain.clearAllEthereumPermissions()
  }

  func resetTransaction() {
    txService.reset()
  }

  public func isDefaultKeyringCreated(_ completion: @escaping (Bool) -> Void) {
    keyringService.defaultKeyringInfo { keyring in
      completion(keyring.isKeyringCreated)
    }
  }

  public func addKeyringServiceObserver(_ observer: BraveWalletKeyringServiceObserver) {
    keyringService.add(observer)
  }
}
