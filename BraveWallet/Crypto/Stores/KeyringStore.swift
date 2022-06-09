/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import BraveShared
import Security
import Strings
import LocalAuthentication
import Combine

struct AutoLockInterval: Identifiable, Hashable {
  var value: Int32
  var label: String

  init(value: Int32) {
    self.value = value
    self.label = Self.formatter.string(
      from: Measurement<UnitDuration>(value: Double(value), unit: .minutes)
    )
  }

  var id: Int32 {
    value
  }

  static let minute = AutoLockInterval(value: 1)
  static let fiveMinutes = AutoLockInterval(value: 5)
  static let tenMinutes = AutoLockInterval(value: 10)
  static let thirtyMinutes = AutoLockInterval(value: 30)

  static let allOptions: [AutoLockInterval] = [
    .minute,
    .fiveMinutes,
    .tenMinutes,
    .thirtyMinutes,
  ]

  private static let formatter = MeasurementFormatter().then {
    $0.locale = Locale.current
    $0.unitOptions = .providedUnit
    $0.unitStyle = .long
  }
}

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
/// This wraps a KeyringService that you would obtain through BraveCore and makes it observable
public class KeyringStore: ObservableObject {
  /// The users current keyring information. By default this is an empty keyring which has no accounts.
  @Published private(set) var keyring: BraveWallet.KeyringInfo = .init()
  /// Whether or not the user should be viewing the onboarding flow to setup a keyring
  @Published private(set) var isOnboardingVisible: Bool = false
  /// Whether or not the last time the wallet was locked was due to the user manually locking it
  ///
  /// Affects whether or not we automatically fill the password from the keychain immediately on
  /// `UnlockWalletView` appearing
  @Published private(set) var lockedManually: Bool = false
  /// Whether or not the biometrics prompt is currently visible in the unlock view
  ///
  /// Needed as a workaround that we show the prompt _after_ the restore completes, which unlocks the wallet
  /// and dismisses the `UnlockWalletView`. We need to keep the unlock view/biometrics prompt up until
  /// the user dismisess it or enables it.
  @Published var isRestoreFromUnlockBiometricsPromptVisible: Bool = false
  /// The users selected account when buying/sending/swapping currencies
  @Published var selectedAccount: BraveWallet.AccountInfo = .init() {
    didSet {
      if oldValue.address == selectedAccount.address { return }
      keyringService.setSelectedAccount(selectedAccount.address, coin: .eth) { _ in }
    }
  }

  private let keyringService: BraveWalletKeyringService
  private var cancellable: AnyCancellable?
  private let keychain: KeychainType

  public init(
    keyringService: BraveWalletKeyringService,
    keychain: KeychainType = Keychain()
  ) {
    self.keyringService = keyringService
    self.keychain = keychain
    
    self.keyringService.add(self)
    updateKeyringInfo()
    self.keyringService.defaultKeyringInfo { [self] keyringInfo in
      isOnboardingVisible = !keyringInfo.isKeyringCreated
      if isKeychainPasswordStored && isOnboardingVisible {
        // If a user deletes the app and they had a stored user password in the past that keychain item
        // stays persisted. When we grab the keyring for the first time we should check to see if they have
        // a wallet created _and_ also have a password stored because if they do then it is no longer valid
        // and should be removed.
        resetKeychainStoredPassword()
      }
    }
    cancellable = NotificationCenter.default
      .publisher(for: UIApplication.didBecomeActiveNotification, object: nil)
      .sink { [weak self] _ in
        self?.updateKeyringInfo()
      }
  }

  private func updateKeyringInfo() {
    keyringService.defaultKeyringInfo { [self] keyringInfo in
      if UIApplication.shared.applicationState != .active {
        // Changes made in the backgroud due to timers going off at launch don't
        // re-render things properly.
        //
        // This function is called again on `didBecomeActiveNotification` anyways.
        return
      }
      keyring = keyringInfo
      if !keyring.accountInfos.isEmpty {
        keyringService.selectedAccount(.eth) { [self] accountAddress in
          selectedAccount = keyringInfo.accountInfos.first(where: { $0.address == accountAddress }) ?? keyringInfo.accountInfos.first!
        }
      }
    }
  }

  func markOnboardingCompleted() {
    self.isOnboardingVisible = false
  }

  func notifyWalletBackupComplete() {
    keyringService.notifyWalletBackupComplete()
  }

  func lock() {
    lockedManually = true
    keyringService.lock()
  }

  func unlock(password: String, completion: @escaping (Bool) -> Void) {
    if !keyring.isKeyringCreated {
      completion(false)
      return
    }
    keyringService.unlock(password) { [weak self] unlocked in
      completion(unlocked)
      if unlocked {
        // Reset this state for next unlock
        self?.lockedManually = false
      }
      self?.updateKeyringInfo()
    }
  }
  
  func validate(password: String, completion: @escaping (Bool) -> Void) {
    if !keyring.isKeyringCreated {
      completion(false)
      return
    }
    keyringService.validatePassword(password, completion: completion)
  }

  func isStrongPassword(_ password: String, completion: @escaping (Bool) -> Void) {
    keyringService.isStrongPassword(password, completion: completion)
  }

  func createWallet(password: String, completion: ((String) -> Void)? = nil) {
    keyringService.createWallet(password) { [weak self] mnemonic in
      self?.updateKeyringInfo()
      completion?(mnemonic)
    }
  }

  func recoveryPhrase(_ completion: @escaping ([RecoveryWord]) -> Void) {
    keyringService.mnemonic { phrase in
      let words =
        phrase
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
    keyringService.restoreWallet(
      phrase,
      password: password,
      isLegacyBraveWallet: isLegacyBraveWallet
    ) { [weak self] isMnemonicValid in
      guard let self = self else { return }
      if isMnemonicValid {
        // Restoring from wallet means you already have your phrase backed up
        self.notifyWalletBackupComplete()
        self.updateKeyringInfo()
        self.resetKeychainStoredPassword()
      }
      completion?(isMnemonicValid)
    }
  }

  func addPrimaryAccount(_ name: String, completion: ((Bool) -> Void)? = nil) {
    keyringService.addAccount(name, coin: .eth) { success in
      self.updateKeyringInfo()
      completion?(success)
    }
  }

  func addSecondaryAccount(_ name: String, privateKey: String, completion: ((Bool, String) -> Void)? = nil) {
    keyringService.importAccount(name, privateKey: privateKey, coin: .eth) { success, address in
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
    keyringService.importAccount(fromJson: name, password: password, json: json) { success, address in
      completion?(success, address)
    }
  }

  func removeSecondaryAccount(forAddress address: String, completion: ((Bool) -> Void)? = nil) {
    keyringService.removeImportedAccount(address, coin: .eth) { success in
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
      keyringService.setKeyringImportedAccountName(BraveWallet.DefaultKeyringId, address: account.address, name: name, completion: handler)
    } else {
      keyringService.setKeyringDerivedAccountName(BraveWallet.DefaultKeyringId, address: account.address, name: name, completion: handler)
    }
  }

  func privateKey(for account: BraveWallet.AccountInfo, completion: @escaping (String?) -> Void) {
    if account.isPrimary {
      keyringService.privateKey(forKeyringAccount: account.address, coin: .eth) { success, key in
        completion(success ? key : nil)
      }
    } else {
      keyringService.privateKey(forImportedAccount: account.address, coin: .eth) { success, key in
        completion(success ? key : nil)
      }
    }
  }

  func notifyUserInteraction() {
    if keyring.isLocked {
      // Auto-lock isn't running until the keyring is unlocked
      return
    }
    keyringService.notifyUserInteraction()
  }

  // MARK: - Keychain

  static let passwordKeychainKey = "brave-wallet-password"

  /// Stores the users wallet password in the keychain so that they may unlock using biometrics/passcode
  func storePasswordInKeychain(_ password: String) -> OSStatus {
    keychain.storePasswordInKeychain(key: Self.passwordKeychainKey, password: password)
  }

  @discardableResult
  func resetKeychainStoredPassword() -> Bool {
    keychain.resetPasswordInKeychain(key: Self.passwordKeychainKey)
  }

  var isKeychainPasswordStored: Bool {
    keychain.isPasswordStoredInKeychain(key: Self.passwordKeychainKey)
  }

  /// Retreives a users stored wallet password using biometrics or passcode or nil if they have not saved
  /// it to the keychain
  func retrievePasswordFromKeychain() -> String? {
    keychain.getPasswordFromKeychain(key: Self.passwordKeychainKey)
  }
}

extension KeyringStore: BraveWalletKeyringServiceObserver {
  public func keyringReset() {
    isOnboardingVisible = true
  }

  public func autoLockMinutesChanged() {
  }

  public func selectedAccountChanged(_ coinType: BraveWallet.CoinType) {
    updateKeyringInfo()
  }

  public func keyringCreated(_ keyringId: String) {
    updateKeyringInfo()
  }

  public func keyringRestored(_ keyringId: String) {
    updateKeyringInfo()
  }

  public func locked() {
    // Put this in the background since biometrics prompt will block the main queue
    DispatchQueue.main.async { [self] in
      self.updateKeyringInfo()
    }
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
