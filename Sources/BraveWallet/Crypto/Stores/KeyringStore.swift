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
import Preferences

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
    $0.unitStyle = .medium
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

/// Validate if password is weak/medium/strong
enum PasswordStatus: Equatable {
  case none // empty
  case invalid // less than 8
  case weak // between 8 to 12
  case medium // more than 12
  case strong // more than 16
  
  var description: String {
    switch self {
    case .none:
      return ""
    case .invalid, .weak:
      return Strings.Wallet.passwordStatusWeak
    case .medium:
      return Strings.Wallet.passwordStatusMedium
    case .strong:
      return Strings.Wallet.passwordStatusStrong
    }
  }
  
  var tintColor: Color {
    switch self {
    case .none:
      return Color.clear
    case .invalid, .weak:
      return Color(uiColor: WalletV2Design.passwordWeakRed)
    case .medium:
      return Color(uiColor: WalletV2Design.passwordMediumYellow)
    case .strong:
      return Color(uiColor: WalletV2Design.passwordStrongGreen)
    }
  }
  
  var percentage: CGFloat {
    switch self {
    case .none:
      return 0
    case .invalid, .weak:
      return 1 / 3
    case .medium:
      return 2 / 3
    case .strong:
      return 1
    }
  }
}

/// An interface that helps you interact with a users keyring
///
/// This wraps a KeyringService that you would obtain through BraveCore and makes it observable
public class KeyringStore: ObservableObject, WalletObserverStore {
  /// The defualt keyring information. By default this is an empty keyring which has no accounts.
  @Published private(set) var defaultKeyring: BraveWallet.KeyringInfo = .init(
    id: .default,
    isKeyringCreated: false,
    isLocked: true,
    isBackedUp: false,
    accountInfos: []
  )
  /// A boolean indciates front-end has or has not loaded Keyring from the core
  @Published var isDefaultKeyringLoaded = false
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
  
  var passwordToSaveInBiometric: String?
  
  /// The origin of the active tab (if applicable). Used for fetching/selecting network for the DApp origin.
  public var origin: URLOrigin?
  
  /// If this KeyringStore instance is creating a wallet.
  /// This flag is used to know when to dismiss onboarding when multiple windows are visible.
  private var isCreatingWallet = false
  /// If this KeyringStore instance is restoring a wallet.
  /// This flag is used to know when to dismiss onboarding when multiple windows are visible.
  private var isRestoringWallet = false

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private var cancellable: AnyCancellable?
  private let keychain: KeychainType
  private var keyringServiceObserver: KeyringServiceObserver?
  private var rpcServiceObserver: JsonRpcServiceObserver?
  
  var isObserving: Bool {
    keyringServiceObserver != nil && rpcServiceObserver != nil
  }

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
    
    self.setupObservers()
    
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
  
  public func setupObservers() {
    guard !isObserving else { return }
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _keyringReset: { [weak self] in
        self?.isOnboardingVisible = true
        self?.updateKeyringInfo()
      }, 
      _keyringCreated: { [weak self] keyringId in
        guard let self else { return }
        if self.isOnboardingVisible, !self.isCreatingWallet, keyringId == BraveWallet.KeyringId.default {
          // Another window has created a wallet. We should dismiss onboarding on this
          // window and allow the other window to continue with it's onboarding flow.
          self.isOnboardingVisible = false
        }
        
        Task { @MainActor in
          let newKeyring = await self.keyringService.keyringInfo(keyringId)
          let selectedAccount = await self.keyringService.allAccounts().selectedAccount
          // if the new Keyring doesn't have a selected account, select the first account
          if selectedAccount == nil, let newAccount = newKeyring.accountInfos.first {
            await self.keyringService.setSelectedAccount(newAccount.accountId)
          }
          self.updateKeyringInfo()
        }
      },
      _keyringRestored: { [weak self] keyringId in
        guard let self else { return }
        if self.isOnboardingVisible && !self.isRestoringWallet, keyringId == BraveWallet.KeyringId.default {
          // Another window has restored a wallet. We should dismiss onboarding on this
          // window and allow the other window to continue with it's onboarding flow.
          self.isOnboardingVisible = false
        }
        
        self.updateKeyringInfo()
      },
      _locked: { [weak self] in
        // Put this in the background since biometrics prompt will block the main queue
        DispatchQueue.main.async {
          self?.updateKeyringInfo()
        }
      },
      _unlocked: { [weak self] in
        self?.updateKeyringInfo()
      },
      _backedUp: { [weak self] in
        self?.updateKeyringInfo()
      },
      _accountsChanged: { [weak self] in
        self?.updateKeyringInfo()
      }, 
      _selectedWalletAccountChanged: { [weak self] _ in
        self?.updateKeyringInfo()
      },
      _selectedDappAccountChanged: { [weak self] _, _ in
        self?.updateKeyringInfo()
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        self?.updateKeyringInfo()
      }
    )
  }
  
  public func tearDown() {
    keyringServiceObserver = nil
    rpcServiceObserver = nil
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
      let selectedAccount = await keyringService.allAccounts().selectedAccount
      let selectedAccountAddress = selectedAccount?.address
      let allKeyrings = await keyringService.keyrings(for: WalletConstants.supportedCoinTypes())
      if let defaultKeyring = allKeyrings.first(where: { $0.id == BraveWallet.KeyringId.default }) {
        self.defaultKeyring = defaultKeyring
        self.isDefaultKeyringLoaded = true
        self.isDefaultKeyringCreated = defaultKeyring.isKeyringCreated
        // fallback case where user completed front-end onboarding, but has no keyring created/accounts.
        if !defaultKeyring.isKeyringCreated && Preferences.Wallet.isOnboardingCompleted.value {
          Preferences.Wallet.isOnboardingCompleted.reset()
        }
      }
      self.allKeyrings = allKeyrings
      if let selectedAccountKeyring = allKeyrings.first(where: { $0.id == selectedAccount?.keyringId }) {
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
        let currentlySelectedAccount = await keyringService.allAccounts().selectedAccount
        guard currentlySelectedAccount?.accountId.uniqueKey != account.accountId.uniqueKey else {
          // account is already selected
          return
        }
        let success = await keyringService.setSelectedAccount(account.accountId)
        if success {
          self.selectedAccount = account
        }
      }
  }

  func markOnboardingCompleted() {
    self.isCreatingWallet = false
    self.isRestoringWallet = false
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

  func validatePassword(_ password: String, completion: @escaping (PasswordStatus) -> Void) {
    if password.count >= 16 {
      completion(.strong)
    } else if password.count >= 12 {
      completion(.medium)
    } else if password.isEmpty {
      completion(.none)
    } else if password.count >= 8 {
      completion(.weak)
    } else {
      completion(.invalid)
    }
  }
  
  @MainActor func validatePassword(_ password: String) async -> PasswordStatus {
    await withCheckedContinuation { continuation in
      validatePassword(password) { isStrong in
        continuation.resume(returning: isStrong)
      }
    }
  }

  func createWallet(password: String, completion: ((String) -> Void)? = nil) {
    isCreatingWallet = true
    keyringService.createWallet(password) { [weak self] mnemonic in
      self?.updateKeyringInfo()
      if !mnemonic.isEmpty {
        self?.passwordToSaveInBiometric = password
      }
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
    isRestoringWallet = true
    keyringService.restoreWallet(
      phrase,
      password: password,
      isLegacyBraveWallet: isLegacyBraveWallet
    ) { [weak self] isMnemonicValid in
      guard let self = self else { return }
      if isMnemonicValid {
        // Restoring from wallet means you already have your phrase backed up
        self.passwordToSaveInBiometric = password
        self.notifyWalletBackupComplete()
        self.updateKeyringInfo()
        self.resetKeychainStoredPassword()
      }
      for coin in WalletConstants.supportedCoinTypes(.dapps) { // only coin types support dapps have permission management 
        Domain.clearAllWalletPermissions(for: coin)
      }
      Preferences.Wallet.sortOrderFilter.reset()
      Preferences.Wallet.isHidingSmallBalancesFilter.reset()
      Preferences.Wallet.isHidingUnownedNFTsFilter.reset()
      Preferences.Wallet.isShowingNFTNetworkLogoFilter.reset()
      Preferences.Wallet.nonSelectedAccountsFilter.reset()
      Preferences.Wallet.nonSelectedNetworksFilter.reset()
      completion?(isMnemonicValid)
    }
  }

  /// `chainId` is only for .fil or .btc coin type
  /// correct `BraveWallet.KeyringId` will be returned from `keyringIdForNewAccount`
  func addPrimaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    chainId: String,
    completion: ((Bool) -> Void)? = nil
  ) {
    keyringService.addAccount(
      coin,
      keyringId: BraveWallet.KeyringId.keyringId(for: coin, on: chainId),
      accountName: name
    ) { accountInfo in
      self.updateKeyringInfo()
      completion?(accountInfo != nil)
    }
  }

  /// `chainId` is only for .fil or .btc coin type
  func addSecondaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    chainId: String,
    privateKey: String,
    completion: ((BraveWallet.AccountInfo?) -> Void)? = nil
  ) {
    if coin == .fil {
      keyringService.importFilecoinAccount(name, privateKey: privateKey, network: chainId) { accountInfo in
        completion?(accountInfo)
      }
    } else {
      keyringService.importAccount(name, privateKey: privateKey, coin: coin) { accountInfo in
        self.updateKeyringInfo()
        completion?(accountInfo)
      }
    }
  }

  func addSecondaryAccount(
    _ name: String,
    json: String,
    password: String,
    completion: ((BraveWallet.AccountInfo?) -> Void)? = nil
  ) {
    keyringService.importAccount(fromJson: name, password: password, json: json) { accountInfo in
      completion?(accountInfo)
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
  
  @MainActor func selectedDappAccount(for coin: BraveWallet.CoinType) async -> BraveWallet.AccountInfo? {
    let allAccounts = await keyringService.allAccounts()
    switch coin {
    case .eth:
      return allAccounts.ethDappSelectedAccount
    case .sol:
      return allAccounts.solDappSelectedAccount
    default:
      return nil
    }
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
