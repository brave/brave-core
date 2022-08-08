// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import LocalAuthentication
import BraveCore
import Data
import BraveShared

public class SettingsStore: ObservableObject {
  /// The number of minutes to wait until the Brave Wallet is automatically locked
  @Published var autoLockInterval: AutoLockInterval = .minute {
    didSet {
      keyringService.setAutoLockMinutes(autoLockInterval.value) { _ in }
    }
  }

  /// If we should attempt to unlock via biometrics (Face ID / Touch ID)
  var isBiometricsUnlockEnabled: Bool {
    keychain.isPasswordStoredInKeychain(key: KeyringStore.passwordKeychainKey) && isBiometricsAvailable
  }

  /// If the device has biometrics available
  var isBiometricsAvailable: Bool {
    LAContext().canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  }

  /// The current default base currency code
  @Published var currencyCode: CurrencyCode = .usd {
    didSet {
      walletService.setDefaultBaseCurrency(currencyCode.code)
    }
  }

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let txService: BraveWalletTxService
  private let keychain: KeychainType

  public init(
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    keychain: KeychainType = Keychain()
  ) {
    self.keyringService = keyringService
    self.walletService = walletService
    self.txService = txService
    self.keychain = keychain

    keyringService.add(self)
    keyringService.autoLockMinutes { [self] minutes in
      self.autoLockInterval = .init(value: minutes)
    }
    
    walletService.add(self)
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = CurrencyCode(code: currencyCode)
    }
  }

  func reset() {
    walletService.reset()
    keychain.resetPasswordInKeychain(key: KeyringStore.passwordKeychainKey)
    Domain.clearAllEthereumPermissions()
    Preferences.Wallet.defaultWallet.reset()
    Preferences.Wallet.allowEthereumProviderAccountRequests.reset()
    Preferences.Wallet.displayWeb3Notifications.reset()
  }

  func resetTransaction() {
    txService.reset()
  }

  public func isDefaultKeyringCreated(_ completion: @escaping (Bool) -> Void) {
    keyringService.keyringInfo(BraveWallet.DefaultKeyringId) { keyring in
      completion(keyring.isKeyringCreated)
    }
  }

  public func addKeyringServiceObserver(_ observer: BraveWalletKeyringServiceObserver) {
    keyringService.add(observer)
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
}

extension SettingsStore: BraveWalletKeyringServiceObserver {
  public func keyringCreated(_ keyringId: String) {
  }
  
  public func keyringRestored(_ keyringId: String) {
  }
  
  public func keyringReset() {
  }
  
  public func locked() {
  }
  
  public func unlocked() {
  }
  
  public func backedUp() {
  }
  
  public func accountsChanged() {
  }
  
  public func autoLockMinutesChanged() {
    keyringService.autoLockMinutes { [weak self] minutes in
      self?.autoLockInterval = .init(value: minutes)
    }
  }
  
  public func selectedAccountChanged(_ coin: BraveWallet.CoinType) {
  }
}

extension SettingsStore: BraveWalletBraveWalletServiceObserver {
  public func onActiveOriginChanged(_ originInfo: BraveWallet.OriginInfo) {
  }
  
  public func onDefaultWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDefaultBaseCurrencyChanged(_ currency: String) {
    currencyCode = CurrencyCode(code: currency)
  }
  
  public func onDefaultBaseCryptocurrencyChanged(_ cryptocurrency: String) {
  }
  
  public func onNetworkListChanged() {
  }
  
  public func onDefaultEthereumWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDefaultSolanaWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
}

struct CurrencyCode: Hashable, Identifiable {
  let code: String
  var id: String { code }
  
  init(code: String) {
    self.code = code
  }
  
  static let aed: Self = .init(code: "AED")
  static let ars: Self = .init(code: "ARS")
  static let aud: Self = .init(code: "AUD")
  static let bdt: Self = .init(code: "BDT")
  static let bhd: Self = .init(code: "BHD")
  static let bmd: Self = .init(code: "BMD")
  static let brl: Self = .init(code: "BRL")
  static let cad: Self = .init(code: "CAD")
  static let chf: Self = .init(code: "CHF")
  static let clp: Self = .init(code: "CLP")
  static let czk: Self = .init(code: "CZK")
  static let dkk: Self = .init(code: "DKK")
  static let eur: Self = .init(code: "EUR")
  static let gbp: Self = .init(code: "GBP")
  static let hkd: Self = .init(code: "HKD")
  static let huf: Self = .init(code: "HUF")
  static let idr: Self = .init(code: "IDR")
  static let ils: Self = .init(code: "ILS")
  static let inr: Self = .init(code: "INR")
  static let jpy: Self = .init(code: "JPY")
  static let krw: Self = .init(code: "KRW")
  static let kwd: Self = .init(code: "KWD")
  static let lkr: Self = .init(code: "LKR")
  static let mmk: Self = .init(code: "MMK")
  static let mxn: Self = .init(code: "MXN")
  static let myr: Self = .init(code: "MYR")
  static let ngn: Self = .init(code: "NGN")
  static let nok: Self = .init(code: "NOK")
  static let nzd: Self = .init(code: "NZD")
  static let php: Self = .init(code: "PHP")
  static let pkr: Self = .init(code: "PKR")
  static let pln: Self = .init(code: "PLN")
  static let rub: Self = .init(code: "RUB")
  static let sar: Self = .init(code: "SAR")
  static let sek: Self = .init(code: "SEK")
  static let sgd: Self = .init(code: "SGD")
  static let thb: Self = .init(code: "THB")
  static let `try`: Self = .init(code: "TRY")
  static let twd: Self = .init(code: "TWD")
  static let uah: Self = .init(code: "UAH")
  static let usd: Self = .init(code: "USD")
  static let vef: Self = .init(code: "VEF")
  static let vnd: Self = .init(code: "VND")
  static let zap: Self = .init(code: "ZAR")
  static let xag: Self = .init(code: "XAG")
  static let xau: Self = .init(code: "XAU")
  static let xdr: Self = .init(code: "XDR")
  
  static let allCurrencyCodes: [CurrencyCode] = [aed, ars, aud, bdt, bhd, bmd, brl, cad, chf, clp, czk, dkk, eur, gbp, hkd, huf, idr, ils, inr, jpy, krw, kwd, lkr, mmk, mxn, myr, ngn, nok, nzd, php, pkr, pln, rub, sar, sek, sgd, thb, `try`, twd, uah, usd, vef, vnd, zap, xag, xau, xdr]
}
