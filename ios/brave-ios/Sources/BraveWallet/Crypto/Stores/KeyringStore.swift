// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Combine
import Data
import Foundation
import LocalAuthentication
import Preferences
import Security
import Strings
import SwiftUI

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
  case none  // empty
  case invalid  // less than 8
  case weak  // between 8 to 12
  case medium  // more than 12
  case strong  // more than 16

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
  /// A boolean indciates front-end has or has not loaded accounts from the core
  @Published var isLoaded = false
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
      guard oldValue.accountId != selectedAccount.accountId,  // Same account
        !oldValue.accountId.uniqueKey.isEmpty  // initializing `KeyringStore`
      else {
        return
      }
      setSelectedAccount(to: selectedAccount)
    }
  }
  /// Indicates if the wallet has been created. This value used for display wallet related settings if created.
  @Published var isWalletCreated: Bool = false
  /// Indicates if the wallet has been locked.
  @Published var isWalletLocked: Bool = true
  /// Indicates if the wallet has been backed up.
  @Published var isWalletBackedUp: Bool = false
  /// All `AccountInfo` for all available keyrings
  @Published var allAccounts: [BraveWallet.AccountInfo] = []

  /// A list of default account with all support coin types
  @Published var defaultAccounts: [BraveWallet.AccountInfo] = []

  var passwordToSaveInBiometric: String?

  /// The origin of the active tab (if applicable). Used for fetching/selecting network for the DApp origin.
  public var origin: URLOrigin?

  /// If this KeyringStore instance is creating a wallet.
  /// Note: Flag is reset prior to onboarding completion step.
  @Published var isCreatingWallet = false
  /// If this KeyringStore instance is restoring a wallet.
  /// Note: Flag is reset prior to onboarding completion step.
  @Published var isRestoringWallet = false
  /// If this KeyringStore instance is creating a wallet or restoring a wallet.
  /// This flag is used to know when to dismiss onboarding when multiple windows are visible.
  private var isOnboarding: Bool = false

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let walletP3A: BraveWalletBraveWalletP3A
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
    walletP3A: BraveWalletBraveWalletP3A,
    keychain: KeychainType = Keychain()
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.walletP3A = walletP3A
    self.keychain = keychain

    setupObservers()

    Preferences.Wallet.isBitcoinTestnetEnabled.observe(from: self)

    updateInfo()

    Task { @MainActor in
      let isWalletCreated = await keyringService.isWalletCreated()
      self.isOnboardingVisible = !isWalletCreated
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
        self?.updateInfo()
      }
  }

  public func setupObservers() {
    guard !isObserving else { return }
    Task { @MainActor in
      // For case where Wallet is dismissed while wallet is being created.
      // Ex. User creates wallet, dismisses & re-opens Wallet before
      // create wallet completion callback. Callback is held with
      // strong ref which keeps `KeyringStore` alive.
      let isWalletCreated = await keyringService.isWalletCreated()
      self.isOnboardingVisible = !isWalletCreated
    }
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _walletReset: { [weak self] in
        self?.isOnboardingVisible = true
        self?.updateInfo()
      },
      _walletCreated: { [weak self] in
        guard let self else { return }
        if self.isOnboardingVisible, !self.isOnboarding {
          // Another window has created a wallet. We should dismiss onboarding on this
          // window and allow the other window to continue with it's onboarding flow.
          self.isOnboardingVisible = false
        }

        self.updateInfo()
      },
      _walletRestored: { [weak self] in
        guard let self else { return }
        if self.isOnboardingVisible && !self.isOnboarding {
          // Another window has restored a wallet. We should dismiss onboarding on this
          // window and allow the other window to continue with it's onboarding flow.
          self.isOnboardingVisible = false
        }

        self.updateInfo()
      },
      _locked: { [weak self] in
        // Put this in the background since biometrics prompt will block the main queue
        DispatchQueue.main.async {
          self?.updateInfo()
        }
      },
      _unlocked: { [weak self] in
        self?.updateInfo()
      },
      _backedUp: { [weak self] in
        self?.updateInfo()
      },
      _accountsChanged: { [weak self] in
        self?.updateInfo()
      },
      _selectedWalletAccountChanged: { [weak self] _ in
        self?.updateInfo()
      },
      _selectedDappAccountChanged: { [weak self] _, _ in
        self?.updateInfo()
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        self?.updateInfo()
      }
    )
  }

  public func tearDown() {
    Task { @MainActor in
      // For case where Wallet is dismissed while wallet is being created.
      // Ex. User creates wallet, dismisses & re-opens Wallet before
      // create wallet completion callback. Callback is held with
      // strong ref which keeps `KeyringStore` alive.
      let isWalletCreated = await keyringService.isWalletCreated()
      self.isOnboardingVisible = !isWalletCreated
      if isRestoringWallet && isWalletCreated {
        // user dismissed wallet while restoring, but after wallet was created in core.
        keyringService.notifyWalletBackupComplete()
        self.isWalletBackedUp = await keyringService.isWalletBackedUp()
      }
    }
    keyringServiceObserver = nil
    rpcServiceObserver = nil
  }

  func updateInfo(completion: (() -> Void)? = nil) {
    Task { @MainActor in
      await updateInfo()
      completion?()
    }
  }

  @MainActor func updateInfo() async {
    if UIApplication.shared.applicationState != .active {
      // Changes made in the backgroud due to timers going off at launch don't
      // re-render things properly.
      //
      // This function is called again on `didBecomeActiveNotification` anyways.
      return
    }
    let allAccounts = await keyringService.allAccounts()
    self.allAccounts = allAccounts.accounts
    if let selectedAccount = allAccounts.selectedAccount {
      self.selectedAccount = selectedAccount
    }
    self.isWalletCreated = await keyringService.isWalletCreated()
    // fallback case where user completed front-end onboarding, but has no keyring created/accounts.
    // this can occur if we crash prior to saving, ex. brave-ios #8291
    if !isWalletCreated && Preferences.Wallet.isOnboardingCompleted.value {
      Preferences.Wallet.isOnboardingCompleted.reset()
    }
    self.isWalletLocked = await keyringService.isLocked()
    self.isWalletBackedUp = await keyringService.isWalletBackedUp()
    self.isLoaded = true
  }

  private func setSelectedAccount(to account: BraveWallet.AccountInfo) {
    Task { @MainActor in
      let currentlySelectedAccount = await keyringService.allAccounts().selectedAccount
      guard currentlySelectedAccount?.accountId.uniqueKey != account.accountId.uniqueKey else {
        // account is already selected
        return
      }
      let success = await keyringService.setSelectedAccount(accountId: account.accountId)
      if success {
        self.selectedAccount = account
      }
    }
  }

  func markOnboardingCompleted() {
    self.isOnboarding = false
    self.isOnboardingVisible = false
    reportP3AOnboarding(action: .complete)
    updateInfo()
  }

  func reportP3AOnboarding(action: BraveWallet.OnboardingAction) {
    walletP3A.reportOnboardingAction(action)
  }

  func notifyWalletBackupComplete() {
    keyringService.notifyWalletBackupComplete()
  }

  func lock() {
    lockedManually = true
    keyringService.lock()
    isWalletLocked = true
  }

  func unlock(password: String, completion: @escaping (Bool) -> Void) {
    if !isWalletCreated {
      completion(false)
      return
    }
    keyringService.unlock(password: password) { [weak self] unlocked in
      completion(unlocked)
      if unlocked {
        // Reset this state for next unlock
        self?.lockedManually = false
        self?.isWalletLocked = false
      }
      self?.updateInfo()
    }
  }

  func validate(password: String, completion: @escaping (Bool) -> Void) {
    if !isWalletCreated {
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

  func createWallet(password: String, completion: ((String?) -> Void)? = nil) {
    guard !isCreatingWallet else {
      completion?(nil)
      return
    }
    isOnboarding = true
    isCreatingWallet = true
    keyringService.isWalletCreated { [weak self] isWalletCreated in
      guard let self else { return }
      guard !isWalletCreated else {
        // Wallet was created already (possible with multi-window) #8425
        self.isOnboarding = false
        self.isCreatingWallet = false
        // Dismiss onboarding if wallet is already setup
        self.isOnboardingVisible = false
        return
      }
      keyringService.createWallet(password: password) { mnemonic in
        self.isCreatingWallet = false
        self.updateInfo()
        if mnemonic != nil {
          self.passwordToSaveInBiometric = password
        }
        completion?(mnemonic)
      }
    }
  }

  func recoveryPhrase(password: String, completion: @escaping ([RecoveryWord]) -> Void) {
    keyringService.walletMnemonic(password: password) { phrase in
      guard let phrase else {
        completion([])
        return
      }
      let words =
        phrase
        .split(separator: " ")
        .enumerated()
        .map { RecoveryWord(value: String($0.element), index: $0.offset) }
      completion(words)
    }
  }

  func restoreWallet(
    words: [String],
    password: String,
    isLegacyEthSeedFormat: Bool,
    completion: ((Bool) -> Void)? = nil
  ) {
    restoreWallet(
      phrase: words.joined(separator: " "),
      password: password,
      isLegacyEthSeedFormat: isLegacyEthSeedFormat,
      completion: completion
    )
  }

  func restoreWallet(
    phrase: String,
    password: String,
    isLegacyEthSeedFormat: Bool,
    completion: ((Bool) -> Void)? = nil
  ) {
    guard !isRestoringWallet else {  // wallet is already being restored.
      completion?(false)
      return
    }
    isOnboarding = true
    isRestoringWallet = true
    keyringService.restoreWallet(
      mnemonic: phrase,
      password: password,
      isLegacyEthSeedFormat: isLegacyEthSeedFormat
    ) { [weak self] isMnemonicValid in
      guard let self = self else { return }
      self.isRestoringWallet = false
      if isMnemonicValid {
        // Restoring from wallet means you already have your phrase backed up
        self.passwordToSaveInBiometric = password
        self.notifyWalletBackupComplete()
        self.updateInfo()
        self.resetKeychainStoredPassword()
      }
      // only coin types support dapps have permission management
      for coin in WalletConstants.supportedCoinTypes(.dapps) {
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

  @MainActor func addPrimaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    chainId: String
  ) async -> Bool {
    var accountName = name
    if accountName.isEmpty {
      // assign default name
      accountName = defaultAccountName(for: coin, chainId: chainId)
    }
    let accountInfo = await keyringService.addAccount(
      coin: coin,
      keyringId: BraveWallet.KeyringId.keyringId(for: coin, on: chainId),
      accountName: accountName
    )
    await updateInfo()
    return accountInfo != nil
  }

  @MainActor func addSecondaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    chainId: String,
    privateKey: String
  ) async -> BraveWallet.AccountInfo? {
    var accountName = name
    if accountName.isEmpty {
      // assign default name
      accountName = defaultAccountName(for: coin, chainId: chainId)
    }
    let accountInfo: BraveWallet.AccountInfo?
    switch coin {
    case .eth:
      accountInfo = await keyringService.importEthereumAccount(
        accountName: accountName,
        privateKey: privateKey
      )
    case .sol:
      accountInfo = await keyringService.importSolanaAccount(
        accountName: accountName,
        privateKey: privateKey
      )
    case .fil:
      accountInfo = await keyringService.importFilecoinAccount(
        accountName: accountName,
        privateKey: privateKey,
        network: chainId
      )
    case .btc:
      accountInfo = await keyringService.importBitcoinAccount(
        accountName: accountName,
        payload: privateKey,
        network: chainId
      )
    case .zec:
      // ZCash not supported on iOS yet, including account import
      return nil
    default:
      return nil
    }
    await updateInfo()
    return accountInfo
  }

  @MainActor func addSecondaryAccount(
    _ name: String,
    coin: BraveWallet.CoinType,
    chainId: String,
    json: String,
    password: String
  ) async -> BraveWallet.AccountInfo? {
    var accountName = name
    if accountName.isEmpty {
      // assign default name
      accountName = defaultAccountName(for: coin, chainId: chainId)
    }
    let accountInfo = await keyringService.importEthereumAccountFromJson(
      accountName: accountName,
      password: password,
      json: json
    )
    await updateInfo()
    return accountInfo
  }

  private func defaultAccountName(
    for coin: BraveWallet.CoinType,
    chainId: String
  ) -> String {
    let keyringId: BraveWallet.KeyringId = .keyringId(for: coin, on: chainId)
    let numAccountsForKeyring = allAccounts.filter({ $0.keyringId == keyringId }).count
    if WalletConstants.supportedTestNetworkChainIds.contains(chainId) {
      // Testnet account
      return String.localizedStringWithFormat(
        Strings.Wallet.defaultTestnetAccountName,
        coin.localizedTitle,
        numAccountsForKeyring + 1
      )
    } else {
      return String.localizedStringWithFormat(
        Strings.Wallet.defaultAccountName,
        coin.localizedTitle,
        numAccountsForKeyring + 1
      )
    }
  }

  func removeSecondaryAccount(
    for account: BraveWallet.AccountInfo,
    password: String,
    completion: ((Bool) -> Void)? = nil
  ) {
    keyringService.removeAccount(
      accountId: account.accountId,
      password: password
    ) { success in
      completion?(success)
      if success {
        self.updateInfo()
      }
    }
  }

  func renameAccount(
    _ account: BraveWallet.AccountInfo,
    name: String,
    completion: ((Bool) -> Void)? = nil
  ) {
    let handler: (Bool) -> Void = { success in
      self.updateInfo()
      completion?(success)
    }
    keyringService.setAccountName(accountId: account.accountId, name: name, completion: handler)
  }

  func privateKey(
    for account: BraveWallet.AccountInfo,
    password: String,
    completion: @escaping (String?) -> Void
  ) {
    keyringService.encodePrivateKeyForExport(
      accountId: account.accountId,
      password: password,
      completion: { key in
        completion(key)
      }
    )
  }

  func notifyUserInteraction() {
    if isWalletLocked {
      // Auto-lock isn't running until the keyring is unlocked
      return
    }
    keyringService.notifyUserInteraction()
  }

  @MainActor func selectedDappAccount(
    for coin: BraveWallet.CoinType
  ) async -> BraveWallet.AccountInfo? {
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
    var osStatus = keychain.storePasswordInKeychain(
      key: Self.passwordKeychainKey,
      password: password
    )
    if osStatus == errSecDuplicateItem {
      // Duplicate item is possible if brave-browser#36669 occurs again.
      // Remove existing stored password & store updated password.
      resetKeychainStoredPassword()
      osStatus = keychain.storePasswordInKeychain(
        key: Self.passwordKeychainKey,
        password: password
      )
    }
    return osStatus
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

extension KeyringStore: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    if Preferences.Wallet.isBitcoinTestnetEnabled.value == false {
      // user disabled Bitcoin Testnet
      Task { @MainActor in
        let allAccounts = await keyringService.allAccounts()
        if let currentlySelectedAccount = allAccounts.selectedAccount,
          currentlySelectedAccount.keyringId == .bitcoin84Testnet,
          let firstAvailableAccount = allAccounts.accounts.first(where: {
            $0.keyringId != currentlySelectedAccount.keyringId
          })
        {
          // we need to switch to the first available account
          setSelectedAccount(to: firstAvailableAccount)
        }
      }
    }
  }
}
