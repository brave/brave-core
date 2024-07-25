// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Data
import Foundation
import LocalAuthentication
import Preferences

public class SettingsStore: ObservableObject, WalletObserverStore {
  /// The number of minutes to wait until the Brave Wallet is automatically locked
  @Published var autoLockInterval: AutoLockInterval = .minute {
    didSet {
      keyringService.setAutoLockMinutes(autoLockInterval.value) { _ in }
    }
  }

  /// If we should attempt to unlock via biometrics (Face ID / Touch ID)
  @Published var isBiometricsUnlockEnabled: Bool = false

  /// If the device has biometrics available
  var isBiometricsAvailable: Bool {
    LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  }

  private var isPasswordStoredInKeychain: Bool {
    keychain.isPasswordStoredInKeychain(key: KeyringStore.passwordKeychainKey)
  }

  /// The current default base currency code
  @Published var currencyCode: CurrencyCode = .usd {
    didSet {
      walletService.setDefaultBaseCurrency(currencyCode.code)
    }
  }

  /// The current ENS Resolve Method preference (Ask / Enabled / Disabled)
  @Published var ensResolveMethod: BraveWallet.ResolveMethod = .ask {
    didSet {
      guard oldValue != ensResolveMethod else { return }
      rpcService.setEnsResolveMethod(ensResolveMethod)
    }
  }

  /// The current ENS Offchain Resolve Method preference (Ask / Enabled / Disabled)
  @Published var ensOffchainResolveMethod: BraveWallet.ResolveMethod = .ask {
    didSet {
      rpcService.setEnsOffchainLookupResolveMethod(ensOffchainResolveMethod)
    }
  }

  /// The current SNS Resolve Method preference (Ask / Enabled / Disabled)
  @Published var snsResolveMethod: BraveWallet.ResolveMethod = .ask {
    didSet {
      guard oldValue != snsResolveMethod else { return }
      rpcService.setSnsResolveMethod(snsResolveMethod)
    }
  }

  /// The current Unstoppable Domains Resolve Method preference (Ask / Enabled / Disabled)
  @Published var udResolveMethod: BraveWallet.ResolveMethod = .ask {
    didSet {
      guard oldValue != udResolveMethod else { return }
      rpcService.setUnstoppableDomainsResolveMethod(udResolveMethod)
    }
  }

  /// The current preference for enabling NFT discovery (Enabled / Disabled)
  @Published var isNFTDiscoveryEnabled: Bool = false {
    didSet {
      guard oldValue != isNFTDiscoveryEnabled else { return }
      walletService.setNftDiscoveryEnabled(isNFTDiscoveryEnabled)
    }
  }

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  let ipfsApi: IpfsAPI
  private let keychain: KeychainType
  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?

  var isObserving: Bool {
    keyringServiceObserver != nil && walletServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService,
    ipfsApi: IpfsAPI,
    keychain: KeychainType = Keychain()
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.txService = txService
    self.ipfsApi = ipfsApi
    self.keychain = keychain
    self.setupObservers()
  }

  func tearDown() {
    keyringServiceObserver = nil
    walletServiceObserver = nil
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _autoLockMinutesChanged: { [weak self] in
        self?.keyringService.autoLockMinutes { minutes in
          self?.autoLockInterval = .init(value: minutes)
        }
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onDefaultBaseCurrencyChanged: { [weak self] currency in
        self?.currencyCode = CurrencyCode(code: currency)
      }
    )
  }

  func setup() {
    Task { @MainActor in
      let currencyCode = await walletService.defaultBaseCurrency()
      self.currencyCode = CurrencyCode(code: currencyCode)

      let autoLockMinutes = await keyringService.autoLockMinutes()
      self.autoLockInterval = .init(value: autoLockMinutes)

      self.snsResolveMethod = await rpcService.snsResolveMethod()
      self.ensResolveMethod = await rpcService.ensResolveMethod()
      self.ensOffchainResolveMethod = await rpcService.ensOffchainLookupResolveMethod()
      self.udResolveMethod = await rpcService.unstoppableDomainsResolveMethod()

      self.isNFTDiscoveryEnabled = await walletService.nftDiscoveryEnabled()
      self.isBiometricsUnlockEnabled = isPasswordStoredInKeychain && isBiometricsAvailable
    }
  }

  func reset() {
    walletService.reset()

    keychain.resetPasswordInKeychain(key: KeyringStore.passwordKeychainKey)
    for coin in WalletConstants.supportedCoinTypes() {
      Domain.clearAllWalletPermissions(for: coin)
      Preferences.Wallet.reset(for: coin)
    }

    Preferences.Wallet.displayWeb3Notifications.reset()
    Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.reset()
    Preferences.Wallet.migrateWalletUserAssetToCoreCompleted.reset()
    // Portfolio/NFT Filters
    Preferences.Wallet.groupByFilter.reset()
    Preferences.Wallet.sortOrderFilter.reset()
    Preferences.Wallet.isHidingSmallBalancesFilter.reset()
    Preferences.Wallet.isHidingUnownedNFTsFilter.reset()
    Preferences.Wallet.isShowingNFTNetworkLogoFilter.reset()
    Preferences.Wallet.nonSelectedAccountsFilter.reset()
    Preferences.Wallet.nonSelectedNetworksFilter.reset()
    // onboarding
    Preferences.Wallet.isOnboardingCompleted.reset()

    Task { @MainActor in
      await WalletUserAssetGroup.removeAllGroup()
    }
  }

  func resetTransaction() {
    txService.reset()
  }

  public func addKeyringServiceObserver(_ observer: BraveWalletKeyringServiceObserver) {
    keyringService.addObserver(observer)
  }

  private var manageSiteConnectionsStore: ManageSiteConnectionsStore?

  func manageSiteConnectionsStore(keyringStore: KeyringStore) -> ManageSiteConnectionsStore {
    if let manageSiteConnectionsStore = manageSiteConnectionsStore {
      return manageSiteConnectionsStore
    }
    let manageSiteConnectionsStore = ManageSiteConnectionsStore(keyringStore: keyringStore)
    self.manageSiteConnectionsStore = manageSiteConnectionsStore
    return manageSiteConnectionsStore
  }

  func closeManageSiteConnectionStore() {
    manageSiteConnectionsStore = nil
  }

  func updateBiometricsToggle() {
    self.isBiometricsUnlockEnabled = isPasswordStoredInKeychain && isBiometricsAvailable
  }
}

#if DEBUG
// for testing
extension SettingsStore {
  func autoLockMinutesChanged() {
    keyringServiceObserver?.autoLockMinutesChanged()
  }

  func onDefaultBaseCurrencyChanged(_ currency: String) {
    walletServiceObserver?.onDefaultBaseCurrencyChanged(currency: currency)
  }
}
#endif

struct CurrencyCode: Hashable, Identifiable {
  let code: String
  let symbol: String
  var id: String { code }

  init(code: String) {
    self.code = code
    self.symbol = CurrencyCode.symbol(for: code)
  }

  private init(code: String, symbol: String) {
    self.code = code
    self.symbol = symbol
  }

  static let aed: Self = .init(code: "AED", symbol: "د.إ‎")
  static let ars: Self = .init(code: "ARS", symbol: "$")
  static let aud: Self = .init(code: "AUD", symbol: "$")
  static let bdt: Self = .init(code: "BDT", symbol: "৳")
  static let bhd: Self = .init(code: "BHD", symbol: ".د.ب")
  static let bmd: Self = .init(code: "BMD", symbol: "$")
  static let brl: Self = .init(code: "BRL", symbol: "R$")
  static let cad: Self = .init(code: "CAD", symbol: "$")
  static let chf: Self = .init(code: "CHF", symbol: "CHF")
  static let clp: Self = .init(code: "CLP", symbol: "$")
  static let czk: Self = .init(code: "CZK", symbol: "Kč")
  static let dkk: Self = .init(code: "DKK", symbol: "kr")
  static let eur: Self = .init(code: "EUR", symbol: "€")
  static let gbp: Self = .init(code: "GBP", symbol: "£")
  static let hkd: Self = .init(code: "HKD", symbol: "$")
  static let huf: Self = .init(code: "HUF", symbol: "Ft")
  static let idr: Self = .init(code: "IDR", symbol: "Rp")
  static let ils: Self = .init(code: "ILS", symbol: "₪")
  static let inr: Self = .init(code: "INR", symbol: "₹")
  static let jpy: Self = .init(code: "JPY", symbol: "¥")
  static let krw: Self = .init(code: "KRW", symbol: "₩")
  static let kwd: Self = .init(code: "KWD", symbol: "KD")
  static let lkr: Self = .init(code: "LKR", symbol: "₨")
  static let mmk: Self = .init(code: "MMK", symbol: "K")
  static let mxn: Self = .init(code: "MXN", symbol: "$")
  static let myr: Self = .init(code: "MYR", symbol: "RM")
  static let ngn: Self = .init(code: "NGN", symbol: "₦")
  static let nok: Self = .init(code: "NOK", symbol: "kr")
  static let nzd: Self = .init(code: "NZD", symbol: "$")
  static let php: Self = .init(code: "PHP", symbol: "₱")
  static let pkr: Self = .init(code: "PKR", symbol: "₨")
  static let pln: Self = .init(code: "PLN", symbol: "zł")
  static let rub: Self = .init(code: "RUB", symbol: "₽")
  static let sar: Self = .init(code: "SAR", symbol: "﷼")
  static let sek: Self = .init(code: "SEK", symbol: "kr")
  static let sgd: Self = .init(code: "SGD", symbol: "S$")
  static let thb: Self = .init(code: "THB", symbol: "฿")
  static let `try`: Self = .init(code: "TRY", symbol: "₺")
  static let twd: Self = .init(code: "TWD", symbol: "NT$")
  static let uah: Self = .init(code: "UAH", symbol: "₴")
  static let usd: Self = .init(code: "USD", symbol: "$")
  static let vef: Self = .init(code: "VEF", symbol: "Bs")
  static let vnd: Self = .init(code: "VND", symbol: "₫")
  static let xag: Self = .init(code: "XAG", symbol: "XAG")
  static let xau: Self = .init(code: "XAU", symbol: "XAU")
  static let xdr: Self = .init(code: "XDR", symbol: "XDR")
  static let zap: Self = .init(code: "ZAR", symbol: "R")

  static let allCodes: [CurrencyCode] = [
    aed, ars, aud, bdt, bhd, bmd, brl, cad, chf, clp, czk, dkk, eur, gbp, hkd, huf, idr, ils, inr,
    jpy, krw, kwd, lkr, mmk, mxn, myr, ngn, nok, nzd, php, pkr, pln, rub, sar, sek, sgd, thb, `try`,
    twd, uah, usd, vef, vnd, xag, xau, xdr, zap,
  ]

  static func symbol(for currencyCode: String) -> String {
    if let currency = CurrencyCode.allCodes.first(where: {
      $0.code.caseInsensitiveCompare(currencyCode) == .orderedSame
    }) {
      return currency.symbol
    }
    return ""
  }
}
