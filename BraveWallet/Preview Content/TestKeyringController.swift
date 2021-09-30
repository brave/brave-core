/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore

/// A test keyring controller which can be passed to a ``CryptoKeyringStore`` that implements some basic
/// keyring functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``CryptoKeyringStore.previewStore``
class TestKeyringController: NSObject, BraveWalletKeyringController {
  private let keyring: BraveWallet.KeyringInfo = .init()
  private var privateKeys: [String: String] = [:]
  private var password = ""
  // Not a real phrase, has a duplicated word for testing
  private let mnemonic = "pass entire pelican lock repair desert entire cactus actress remain gossip rail"
  private var observers: NSHashTable<BraveWalletKeyringControllerObserver> = .weakObjects()
  
  func add(_ observer: BraveWalletKeyringControllerObserver) {
    observers.add(observer)
  }
  
  // To ensure previews are consistent, use the same set of addresses per run
  private var testAddresses = [
    "0x879240B2D6179E9EC40BC2AFFF",
    "0xB39288B45B55FCBD548D5EF109",
    "0x7275D6B957257A4F15368D8930",
    "0xFF1076C0E2EE29BB4281CF7B40",
    "0xA6E69E94E2CA446FD57A341A49",
    "0xF6FE2940FA26376A42F0DA093E",
    "0xA93904B51AF6C8F3288D7F75C3",
    "0x519221726F9C2239AFAECDA0AA",
    "0x574ABEE83CD184C74DE5AA05F8",
    "0x974CA75B37E25AF6A97CEF8355",
    "0xC238C5F9AC8B178DCB71CC13C0",
    "0x15AC4B2F5AA895F80302C2442B",
    "0x4B6489E181688892382C88520B",
    "0xF50DE302A58ACEFCA6F09D3954",
    "0x04EE3FA0B0FEA6E08B9453025C",
    "0x34ED2D4C49402A51A5A6AC5538",
    "0x07DBAD027F7590D46F50D72177",
    "0x535F73A1D383737C8D23E5E754",
    "0x2816FA7B55A6B5456B0D4E1DB0",
    "0xE03BF40F564F583F42789DFE9C"
  ]
  private var addressIndex: Int = 0
  private func nextAddress() -> String {
    let address = testAddresses[addressIndex]
    addressIndex += 1
    if addressIndex >= testAddresses.endIndex {
      addressIndex = 0
    }
    return address
  }
  
  func addAccount(_ accountName: String, completion: @escaping (Bool) -> Void) {
    let info = BraveWallet.AccountInfo()
    info.name = accountName
    info.address = nextAddress()
    keyring.accountInfos.append(info)
    observers.allObjects.forEach {
      $0.accountsChanged()
    }
    completion(true)
  }
  
  func createWallet(_ password: String, completion: @escaping (String) -> Void) {
    keyring.isDefaultKeyringCreated = true
    keyring.isLocked = false
    self.password = password
    addAccount("Account 1") { _ in }
    observers.allObjects.forEach {
      $0.keyringCreated()
    }
    completion(mnemonic)
  }
  
  func defaultKeyringInfo(_ completion: @escaping (BraveWallet.KeyringInfo) -> Void) {
    completion(keyring)
  }
  
  func isLocked(_ completion: @escaping (Bool) -> Void) {
    completion(keyring.isLocked)
  }
  
  func lock() {
    keyring.isLocked = true
    observers.allObjects.forEach {
      $0.locked()
    }
  }
  
  func isWalletBackedUp(_ completion: @escaping (Bool) -> Void) {
    completion(keyring.isBackedUp)
  }
  
  func mnemonic(forDefaultKeyring completion: @escaping (String) -> Void) {
    completion(mnemonic)
  }
  
  func unlock(_ password: String, completion: @escaping (Bool) -> Void) {
    if !keyring.isDefaultKeyringCreated {
      completion(false)
      return
    }
    let passwordsMatch = self.password == password
    if passwordsMatch {
      keyring.isLocked = false
      observers.allObjects.forEach {
        $0.unlocked()
      }
    }
    completion(passwordsMatch)
  }
  
  func notifyWalletBackupComplete() {
    keyring.isBackedUp = true
    observers.allObjects.forEach {
      $0.backedUp()
    }
  }
  
  func reset() {
    keyring.isBackedUp = false
    keyring.isLocked = false
    keyring.isDefaultKeyringCreated = false
    keyring.isBackedUp = false
    keyring.accountInfos.removeAll()
  }
  
  func restoreWallet(_ mnemonic: String, password: String, isLegacyBraveWallet: Bool, completion: @escaping (Bool) -> Void) {
    reset()
    self.password = password
    // Test store does not test phrase validity
    observers.allObjects.forEach {
      $0.keyringRestored()
    }
    completion(true)
  }
  
  // To ensure previews are consistent, use the same set of addresses per run
  private var importedTestAddresses = [
    "0xB05618F6B379B38B120D8EB4E5",
    "0xDFF7D1EDDE1484D547E8F41BEE",
    "0xB1D205B163D1C6681FD327AE33",
    "0x4C71547BC5433B950EDB336A92",
    "0x6FE03430626DEB934B1E13EA51",
    "0xCC625A5A6511392FA1D40BD970",
    "0x701E8FEDAEE87F868A4899D40D",
    "0x5F95775E02A89F70FF8A81B33C",
    "0x4DADD3FC345C0B7BA559B757F5",
    "0xAAE5BD4958DD443F85B0F3D606",
    "0xF6F9D52299CA8F94FCC0C4BDC4",
    "0xD07C121B7372159875E2B7DE46",
    "0xF1E1EFDE452626CD0FE6703284",
    "0x1F58637A1FD7B71BCBEC8F90A8",
    "0xF854F9456C013B8F75F48D42CE",
    "0xF6CD2E28C549CB0B04F33AB5A5",
    "0xF0CA976FE1A503C88C1E626FC1",
    "0x03B8680DF0B949B7D265177F8E",
    "0x8C0033DEDD3E4A02B5FD22DB1A",
    "0x41B59E6D21216C8FBE3C4C4EAC"
  ]
  private var importedAddressIndex: Int = 0
  private func nextImportedAddress() -> String {
    let address = importedTestAddresses[importedAddressIndex]
    importedAddressIndex += 1
    if importedAddressIndex >= importedTestAddresses.endIndex {
      importedAddressIndex = 0
    }
    return address
  }
  
  func importAccount(_ accountName: String, privateKey: String, completion: @escaping (Bool, String) -> Void) {
    let info = BraveWallet.AccountInfo()
    info.name = accountName
    info.address = nextImportedAddress()
    info.isImported = true
    privateKeys[info.address] = privateKey
    keyring.accountInfos.append(info)
    observers.allObjects.forEach {
      $0.accountsChanged()
    }
    completion(true, info.address)
  }
  
  func importAccount(fromJson accountName: String, password: String, json: String, completion: @escaping (Bool, String) -> Void) {
    completion(false, "")
  }
  
  func privateKey(forDefaultKeyringAccount address: String, completion: @escaping (Bool, String) -> Void) {
    completion(true, "807df4db569fab37cdf475a4bda779897f0f3dd9c5d90a2cb953c88ef762fd96")
  }
  
  func privateKey(forImportedAccount address: String, completion: @escaping (Bool, String) -> Void) {
    if let key = privateKeys[address] {
      completion(true, key)
    } else {
      completion(false, "")
    }
  }
  
  func removeImportedAccount(_ address: String, completion: @escaping (Bool) -> Void) {
    guard let index = keyring.accountInfos.firstIndex(where: { $0.address == address }) else {
      completion(false)
      return
    }
    keyring.accountInfos.remove(at: index)
    observers.allObjects.forEach {
      $0.accountsChanged()
    }
    completion(true)
  }
  
  func setDefaultKeyringDerivedAccountName(_ address: String, name: String, completion: @escaping (Bool) -> Void) {
    if let account = keyring.accountInfos.first(where: { $0.address == address }) {
      account.name = name
      completion(true)
      return
    }
    completion(false)
  }
  
  func setDefaultKeyringImportedAccountName(_ address: String, name: String, completion: @escaping (Bool) -> Void) {
    if let account = keyring.accountInfos.first(where: { $0.address == address && $0.isImported }) {
      account.name = name
      completion(true)
      return
    }
    completion(false)
  }
  
  func addHardwareAccounts(_ info: [BraveWallet.HardwareWalletAccount]) {
    // Hardware wallets not supported on iOS
  }
  
  func hardwareAccounts(_ completion: @escaping ([BraveWallet.AccountInfo]) -> Void) {
    // Hardware wallets not supported on iOS
    completion([])
  }
  
  func removeHardwareAccount(_ address: String) {
    // Hardware wallets not supported on iOS
  }
}
