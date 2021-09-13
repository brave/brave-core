/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore

/// Defines a single word in a bip32 mnemonic recovery prhase.
///
/// Since a single phrase can have duplicate words, index matters in a SwiftUI context
struct RecoveryWord: Hashable, Identifiable {
  /// The word itself
  var value: String
  /// The index of that word within the entire phrase
  var index: Int
  
  var id: String {
    "\(index)-\(value)"
  }
}

/// An interface that helps you interact with a users keyring
///
/// This wraps a KeyringController that you would obtain through BraveCore and makes it observable
public class KeyringStore: ObservableObject {
  /// The users current keyring information. By default this is an empty keyring which has no accounts.
  @Published private(set) var keyring: BraveWallet.KeyringInfo = .init()
  /// Whether or not the user should be viewing the onboarding flow to setup a keyring
  @Published private(set) var isOnboardingVisible: Bool = false
  
  private let controller: BraveWalletKeyringController
  
  public init(keyringController: BraveWalletKeyringController) {
    controller = keyringController
    controller.add(self)
    controller.defaultKeyringInfo { keyringInfo in
      self.keyring = keyringInfo
      self.isOnboardingVisible = !keyringInfo.isDefaultKeyringCreated
    }
  }
  
  private func updateKeyringInfo() {
    controller.defaultKeyringInfo { keyringInfo in
      self.keyring = keyringInfo
    }
  }
  
  func markOnboardingCompleted() {
    self.isOnboardingVisible = false
  }
  
  func notifyWalletBackupComplete() {
    controller.notifyWalletBackupComplete()
  }
  
  func lock() {
    controller.lock()
  }
  
  func unlock(password: String, completion: @escaping (Bool) -> Void) {
    if !keyring.isDefaultKeyringCreated { return }
    controller.unlock(password) { [weak self] unlocked in
      completion(unlocked)
      self?.updateKeyringInfo()
    }
  }
  
  func createWallet(password: String, completion: ((String) -> Void)? = nil) {
    controller.createWallet(password) { [weak self] mnemonic in
      self?.updateKeyringInfo()
      completion?(mnemonic)
    }
  }
  
  func recoveryPhrase(_ completion: @escaping ([RecoveryWord]) -> Void) {
    controller.mnemonic { phrase in
      let words = phrase
        .split(separator: " ")
        .enumerated()
        .map { RecoveryWord(value: String($0.element), index: $0.offset) }
      completion(words)
    }
  }
  
  func restoreWallet(words: [String], password: String, completion: ((Bool) -> Void)? = nil) {
    restoreWallet(phrase: words.joined(separator: " "), password: password, completion: completion)
  }
  
  func restoreWallet(phrase: String, password: String, completion: ((Bool) -> Void)? = nil) {
    controller.restoreWallet(
      phrase,
      password: password
    ) { [weak self] isMnemonicValid in
      if isMnemonicValid {
        // Restoring from wallet means you already have your phrase backed up
        self?.notifyWalletBackupComplete()
        self?.updateKeyringInfo()
      }
      completion?(isMnemonicValid)
    }
  }
  
  func addPrimaryAccount(_ name: String, completion: ((Bool) -> Void)? = nil) {
    controller.addAccount(name) { success in
      self.updateKeyringInfo()
      completion?(success)
    }
  }
  
  func addSecondaryAccount(_ name: String, privateKey: String, completion: ((Bool, String) -> Void)? = nil) {
    controller.importAccount(name, privateKey: privateKey) { success, address in
      self.updateKeyringInfo()
      completion?(success, address)
    }
  }
  
  func addSecondaryAccount(
    _ name: String,
    json: String,
    password: String,
    completion: ((Bool, String) -> Void)? = nil
  ) {
    controller.importAccount(fromJson: name, password: password, json: json) { success, address in
      completion?(success, address)
    }
  }
  
  func removeSecondaryAccount(forAddress address: String, completion: ((Bool) -> Void)? = nil) {
    controller.removeImportedAccount(address) { success in
      self.updateKeyringInfo()
      completion?(success)
    }
  }
  
  func renameAccount(_ account: BraveWallet.AccountInfo, name: String, completion: ((Bool) -> Void)? = nil) {
    let handler: (Bool) -> Void = { success in
      self.updateKeyringInfo()
      completion?(success)
    }
    if account.isImported {
      controller.setDefaultKeyringImportedAccountName(account.address, name: name, completion: handler)
    } else {
      controller.setDefaultKeyringDerivedAccountName(account.address, name: name, completion: handler)
    }
  }
  
  func reset() {
    controller.reset()
  }
}

extension KeyringStore: BraveWalletKeyringControllerObserver {
  public func keyringCreated() {
    updateKeyringInfo()
  }
  
  public func keyringRestored() {
    updateKeyringInfo()
  }
  
  public func locked() {
    updateKeyringInfo()
  }
  
  public func unlocked() {
    updateKeyringInfo()
  }
  
  public func backedUp() {
    updateKeyringInfo()
  }
  
  public func accountsChanged() {
    updateKeyringInfo()
  }  
}
