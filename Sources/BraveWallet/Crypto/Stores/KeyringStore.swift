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
import Data

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
  /// The defualt keyring information. By default this is an empty keyring which has no accounts.
  @Published private(set) var defaultKeyring: BraveWallet.KeyringInfo = .init(
    id: .default,
    isKeyringCreated: false,
    isLocked: true,
    isBackedUp: false,
    accountInfos: []
  )
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
      guard oldValue.address != selectedAccount.address, // Same account
            !oldValue.address.isEmpty // initializing `KeyringStore`
      else {
        return
      }
      setSelectedAccount(to: selectedAccount)
    }
  }
  /// All available `KeyringInfo` for all supported coin type
  @Published var allKeyrings: [BraveWallet.KeyringInfo] = []
  /// Indicates if default keyring has been created. This value used for display wallet related settings if default
  /// keyring has been created
  @Published var isDefaultKeyringCreated: Bool = false
  /// All `AccountInfo` for all available keyrings
  var allAccounts: [BraveWallet.AccountInfo] {
    allKeyrings.flatMap(\.accountInfos)
  }
  
  /// A list of default account with all support coin types
  @Published var defaultAccounts: [BraveWallet.AccountInfo] = []
  
  /// The origin of the active tab (if applicable). Used for fetching/selecting network for the DApp origin.
  public var origin: URLOrigin?
  
  /// Internal flag kept for when `setSelectedAccount` is executing so we can wait for
  /// completion before reacting to observed changes. Ex. chain changed event fires after
  /// `setSelectedAccount` changes network, but before it can set the new account.
  private var isUpdatingSelectedAccount = false {
    didSet {
      if !isUpdatingSelectedAccount {
        // in case the chain did change while we were
        // updating our selected account we should
        // validate our current `selectedAccount`
        updateKeyringInfo()
      }
    }
  }

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private var cancellable: AnyCancellable?
  private let keychain: KeychainType

  public init(
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    keychain: KeychainType = Keychain()
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.keychain = keychain
    
    keyringService.add(self)
    rpcService.add(self)
    updateKeyringInfo()
    
    self.keyringService.keyringInfo(BraveWallet.KeyringId.default) { [self] keyringInfo in
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
    if UIApplication.shared.applicationState != .active {
      // Changes made in the backgroud due to timers going off at launch don't
      // re-render things properly.
      //
      // This function is called again on `didBecomeActiveNotification` anyways.
      return
    }
    Task { @MainActor in // fetch all KeyringInfo for all coin types
      let selectedCoin = await walletService.selectedCoin()
      let selectedAccountAddress = await keyringService.selectedAccount(selectedCoin)
      let allKeyrings = await keyringService.keyrings(for: WalletConstants.supportedCoinTypes)
      self.defaultAccounts = await keyringService.defaultAccounts(for: WalletConstants.supportedCoinTypes)
      if let defaultKeyring = allKeyrings.first(where: { $0.id == BraveWallet.KeyringId.default }) {
        self.defaultKeyring = defaultKeyring
        self.isDefaultKeyringCreated = defaultKeyring.isKeyringCreated
      }
      self.allKeyrings = allKeyrings
      if let selectedAccountKeyring = allKeyrings.first(where: { $0.coin == selectedCoin }) {
        if self.selectedAccount.address != selectedAccountAddress {
          if let selectedAccount = selectedAccountKeyring.accountInfos.first(where: { $0.address == selectedAccountAddress }) {
            self.selectedAccount = selectedAccount
          } else if let firstAccount = selectedAccountKeyring.accountInfos.first {
            // try and correct invalid state (no selected account for this coin type)
            self.selectedAccount = firstAccount
          } // else selected account address does not exist in keyring (should not occur...)
        } // else `self.selectedAccount` is already the currently selected account
      } // else keyring for selected coin is unavailable (should not occur...)
    }
  }
  
  private func setSelectedAccount(to account: BraveWallet.AccountInfo) {
    Task { @MainActor in
      self.isUpdatingSelectedAccount = true
      defer { self.isUpdatingSelectedAccount = false }
      var selectedCoin = await walletService.selectedCoin()
      if selectedCoin != account.coin {
        walletService.setSelectedCoin(account.coin)
        selectedCoin = account.coin
        // Update selected network so `NetworkStore` updates it's network(s).
        // `selectedAccountChanged` doesn't fire when switching coin types,
        // only when selected account for a coin type changes
        let network = await rpcService.network(selectedCoin, origin: nil)
        _ = await rpcService.setNetwork(network.chainId, coin: network.coin, origin: nil)
      }
      let coreSelectedAccount = await keyringService.selectedAccount(selectedCoin)

      if coreSelectedAccount != account.address {
        // Update the selected account in core
        let success = await keyringService.setSelectedAccount(account.accountId)
        if success {
          self.selectedAccount = account
        }
      } else {
        self.selectedAccount = account
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
    if !defaultKeyring.isKeyringCreated {
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
    if !defaultKeyring.isKeyringCreated {
      completion(false)
      return
    }
    keyringService.validatePassword(password, completion: completion)
  }

  func isStrongPassword(_ password: String, completion: @escaping (Bool) -> Void) {
    completion(password.count >= 8)
  }
  
  @MainActor func isStrongPassword(_ password: String) async -> Bool {
    await withCheckedContinuation { continuation in
      isStrongPassword(password) { isStrong in
        continuation.resume(returning: isStrong)
      }
    }
  }

  func createWallet(password: String, completion: ((String) -> Void)? = nil) {
    keyringService.createWallet(password) { [weak self] mnemonic in
      self?.updateKeyringInfo()
      completion?(mnemonic)
    }
  }

  func recoveryPhrase(password: String, completion: @escaping ([RecoveryWord]) -> Void) {
    keyringService.mnemonic(forDefaultKeyring: password) { phrase in
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
      for coin in WalletConstants.supportedCoinTypes {
        Domain.clearAllWalletPermissions(for: coin)
      }
      completion?(isMnemonicValid)
    }
  }

  func addPrimaryAccount(_ name: String, coin: BraveWallet.CoinType, completion: ((Bool) -> Void)? = nil) {
    keyringService.addAccount(
      coin,
      keyringId: coin.keyringId,
      accountName: name
    ) { accountInfo in
      self.updateKeyringInfo()
      completion?(accountInfo != nil)
    }
  }

  func addSecondaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    privateKey: String,
    completion: ((Bool, String) -> Void)? = nil
  ) {
    if coin == .fil {
      rpcService.network(.fil, origin: nil) { [self] defaultNetwork in
        let networkId = defaultNetwork.chainId.caseInsensitiveCompare(BraveWallet.FilecoinMainnet) == .orderedSame ? BraveWallet.FilecoinMainnet : BraveWallet.FilecoinTestnet
        keyringService.importFilecoinAccount(name, privateKey: privateKey, network: networkId) { success, address in
          completion?(success, address)
        }
      }
    } else {
      keyringService.importAccount(name, privateKey: privateKey, coin: coin) { success, address in
        self.updateKeyringInfo()
        completion?(success, address)
      }
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

  func removeSecondaryAccount(for account: BraveWallet.AccountInfo, password: String, completion: ((Bool) -> Void)? = nil) {
    keyringService.removeAccount(
      account.accountId,
      password: password
    ) { success in
      completion?(success)
      if success {
        self.updateKeyringInfo()
      }
    }
  }

  func renameAccount(_ account: BraveWallet.AccountInfo, name: String, completion: ((Bool) -> Void)? = nil) {
    let handler: (Bool) -> Void = { success in
      self.updateKeyringInfo()
      completion?(success)
    }
    keyringService.setAccountName(account.accountId, name: name, completion: handler)
  }

  func privateKey(for account: BraveWallet.AccountInfo, password: String, completion: @escaping (String?) -> Void) {
    keyringService.encodePrivateKey(forExport: account.accountId, password: password, completion: { key in
      completion(key.isEmpty ? nil : key)
    })
  }

  func notifyUserInteraction() {
    if defaultKeyring.isLocked {
      // Auto-lock isn't running until the keyring is unlocked
      return
    }
    keyringService.notifyUserInteraction()
  }
  
  @MainActor func selectedAccount(for coin: BraveWallet.CoinType) async -> BraveWallet.AccountInfo? {
    guard let selectedAccount = await keyringService.selectedAccount(coin) else { return nil }
    return allAccounts.first(where: { $0.address == selectedAccount })
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
    updateKeyringInfo()
  }

  public func autoLockMinutesChanged() {
  }

  public func selectedAccountChanged(_ coinType: BraveWallet.CoinType) {
    walletService.setSelectedCoin(coinType)
    updateKeyringInfo()
  }

  public func keyringCreated(_ keyringId: BraveWallet.KeyringId) {
    Task { @MainActor in
      let newKeyring = await keyringService.keyringInfo(keyringId)
      if let newKeyringCoin = newKeyring.coin {
        let selectedAccount = await keyringService.selectedAccount(newKeyringCoin)
        // if the new Keyring doesn't have a selected account, select the first account
        if selectedAccount == nil, let newAccount = newKeyring.accountInfos.first {
          await keyringService.setSelectedAccount(newAccount.accountId)
        }
      }
      updateKeyringInfo()
    }
  }

  public func keyringRestored(_ keyringId: BraveWallet.KeyringId) {
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
  
  public func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension KeyringStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    walletService.setSelectedCoin(coin)
    if !isUpdatingSelectedAccount {
      // Potential race condition when switching to a non-selected account for new coin type.
      // ex. Sol Account 1 selected for SOL, Eth Account 1 selected for ETH. SOL coin selected.
      // Switching from Sol Account 1 to Eth Account 2, `updateKeyring` may assign Eth Account 1
      // before `setSelectAccount` updates the core selected account to Eth Account 2, causing
      // a potential loop.
      updateKeyringInfo()
    }
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
