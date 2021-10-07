/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import Security

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
  
  func restoreWallet(words: [String], password: String, isLegacyBraveWallet: Bool, completion: ((Bool) -> Void)? = nil) {
    restoreWallet(phrase: words.joined(separator: " "), password: password, isLegacyBraveWallet: isLegacyBraveWallet, completion: completion)
  }
  
  func restoreWallet(phrase: String, password: String, isLegacyBraveWallet: Bool, completion: ((Bool) -> Void)? = nil) {
    controller.restoreWallet(
      phrase,
      password: password,
      isLegacyBraveWallet: isLegacyBraveWallet
    ) { [weak self] isMnemonicValid in
      guard let self = self else { return }
      if isMnemonicValid {
        // Restoring from wallet means you already have your phrase backed up
        self.notifyWalletBackupComplete()
        self.updateKeyringInfo()
        Self.resetKeychainStoredPassword()
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
    isOnboardingVisible = true
    Self.resetKeychainStoredPassword()
  }
  
  func privateKey(for account: BraveWallet.AccountInfo, completion: @escaping (String?) -> Void) {
    if account.isPrimary {
      controller.privateKey(forDefaultKeyringAccount: account.address) { success, key in
        completion(success ? key : nil)
      }
    } else {
      controller.privateKey(forImportedAccount: account.address) { success, key in
        completion(success ? key : nil)
      }
    }
  }
  
  func notifyUserInteraction() {
    if keyring.isLocked {
      // Auto-lock isn't running until the keyring is unlocked
      return
    }
    controller.notifyUserInteraction()
  }
  
  // MARK: - Keychain
  
  private static let passwordKeychainKey = "brave-wallet-password"
  
  /// Stores the users wallet password in the keychain so that they may unlock using biometrics/passcode
  static func storePasswordInKeychain(_ password: String) -> Bool {
    guard let passwordData = password.data(using: .utf8) else { return false }
    let accessControl = SecAccessControlCreateWithFlags(
      nil,
      kSecAttrAccessibleWhenPasscodeSetThisDeviceOnly,
      .userPresence,
      nil
    )
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: passwordKeychainKey,
      kSecAttrAccessControl as String: accessControl as Any,
      kSecValueData as String: passwordData
    ]
    let status = SecItemAdd(query as CFDictionary, nil)
    return status == errSecSuccess
  }
  
  @discardableResult
  static func resetKeychainStoredPassword() -> Bool {
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: passwordKeychainKey
    ]
    let status = SecItemDelete(query as CFDictionary)
    return status == errSecSuccess
  }
  
  /// Retreives a users stored wallet password using biometrics or passcode or nil if they have not saved
  /// it to the keychain
  static func retrievePasswordFromKeychain() -> String? {
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: passwordKeychainKey,
      kSecMatchLimit as String: kSecMatchLimitOne,
      kSecReturnData as String: true
    ]
    var passwordData: AnyObject?
    let status = SecItemCopyMatching(query as CFDictionary, &passwordData)
    guard status == errSecSuccess,
          let data = passwordData as? Data,
          let password = String(data: data, encoding: .utf8) else {
            return nil
          }
    return password
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
