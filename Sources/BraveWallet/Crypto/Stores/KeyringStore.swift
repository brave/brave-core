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
  @Published private(set) var defaultKeyring: BraveWallet.KeyringInfo = .init()
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
      keyringService.setSelectedAccount(selectedAccount.address, coin: selectedAccount.coin) { _ in }
    }
  }
  /// All available `KeyringInfo` for all supported coin type
  @Published var allKeyrings: [BraveWallet.KeyringInfo] = []
  /// All `AccountInfo` for all available keyrings
  var allAccounts: [BraveWallet.AccountInfo] {
    allKeyrings.flatMap(\.accountInfos)
  }
  
  /// A list of default account with all support coin types
  @Published var defaultAccounts: [BraveWallet.AccountInfo] = []

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
    
    self.keyringService.keyringInfo(BraveWallet.DefaultKeyringId) { [self] keyringInfo in
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
      self.allKeyrings = await keyringService.keyrings(for: WalletConstants.supportedCoinTypes)
      self.defaultAccounts = await keyringService.defaultAccounts(for: WalletConstants.supportedCoinTypes)
      if let defaultKeyring = allKeyrings.first(where: { $0.id == BraveWallet.DefaultKeyringId }) {
        self.defaultKeyring = defaultKeyring
      }
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
      self.walletService.setSelectedCoin(.eth)
      self.rpcService.setNetwork(BraveWallet.MainnetChainId, coin: .eth, completion: { _ in })
      Domain.clearAllWalletPermissions(for: .eth)
      // TODO: will need to clear permission for `.sol` coin type once we support solana Dapps
      completion?(isMnemonicValid)
    }
  }

  func addPrimaryAccount(_ name: String, coin: BraveWallet.CoinType, completion: ((Bool) -> Void)? = nil) {
    keyringService.addAccount(name, coin: coin) { success in
      self.updateKeyringInfo()
      completion?(success)
    }
  }

  func addSecondaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    privateKey: String,
    completion: ((Bool, String) -> Void)? = nil
  ) {
    if coin == .fil {
      rpcService.network(.fil) { [self] defaultNetwork in
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

  func removeSecondaryAccount(for account: BraveWallet.AccountInfo, completion: ((Bool) -> Void)? = nil) {
    keyringService.removeImportedAccount(account.address, coin: account.coin) { success in
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
      keyringService.setKeyringImportedAccountName(account.coin.keyringId, address: account.address, name: name, completion: handler)
    } else {
      keyringService.setKeyringDerivedAccountName(account.coin.keyringId, address: account.address, name: name, completion: handler)
    }
  }

  func privateKey(for account: BraveWallet.AccountInfo, completion: @escaping (String?) -> Void) {
    keyringService.privateKey(forKeyringAccount: account.address, coin: account.coin) { success, key in
      completion(success ? key : nil)
    }
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
    Task { @MainActor in
      let previouslySelectedCoin = await walletService.selectedCoin()
      walletService.setSelectedCoin(coinType)
      if previouslySelectedCoin != coinType || selectedAccount.coin != coinType {
        // update network here in case NetworkStore is closed.
        let network = await rpcService.network(coinType)
        await rpcService.setNetwork(network.chainId, coin: coinType)
      }
      updateKeyringInfo()
    }
  }

  public func keyringCreated(_ keyringId: String) {
    Task { @MainActor in
      let newKeyring = await keyringService.keyringInfo(keyringId)
      if let newAccount = newKeyring.accountInfos.first {
        await keyringService.setSelectedAccount(newAccount.address, coin: newAccount.coin)
      }
      updateKeyringInfo()
    }
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

extension KeyringStore: BraveWalletJsonRpcServiceObserver {
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    Task { @MainActor in // This observer will take care of selected account update caused by network switching
      let accountAddress = await keyringService.selectedAccount(coin)
      let keyring = await keyringService.keyringInfo(coin.keyringId)
      if let account = keyring.accountInfos.first(where: { $0.address == accountAddress }) {
        selectedAccount = account
      }
    }
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
