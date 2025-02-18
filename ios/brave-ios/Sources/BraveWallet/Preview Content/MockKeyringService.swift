// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

/// A test keyring controller which can be passed to a ``CryptoKeyringStore`` that implements some basic
/// keyring functionality for the use of SwiftUI Previews.
///
/// - note: Do not use this directly, use ``CryptoKeyringStore.previewStore``
class MockKeyringService: BraveWalletKeyringService {
  private var isWalletCreated = true
  private var isLocked = false
  private var isBackedUp = false
  private var allAccounts: [BraveWallet.AccountInfo] = []
  private var privateKeys: [String: String] = [:]
  private var password = ""
  // Not a real phrase, has a duplicated word for testing
  private let mnemonic =
    "pass entire pelican lock repair desert entire cactus actress remain gossip rail"
  private var observers: NSHashTable<BraveWalletKeyringServiceObserver> = .weakObjects()
  private var selectedAccount: BraveWallet.AccountInfo?

  func addObserver(_ observer: BraveWalletKeyringServiceObserver) {
    observers.add(observer)
  }

  // To ensure previews are consistent, use the same set of addresses per run
  private var testAddresses = [
    "0x879240B2D6179E9EC40BC2AFFF9E9EC40BC2AFFF",
    "0xB39288B45B55FCBD548D5EF109FCBD548D5EF109",
    "0x7275D6B957257A4F15368D89307A4F15368D8930",
    "0xFF1076C0E2EE29BB4281CF7B4029BB4281CF7B40",
    "0xA6E69E94E2CA446FD57A341A49446FD57A341A49",
    "0xF6FE2940FA26376A42F0DA093E376A42F0DA093E",
    "0xA93904B51AF6C8F3288D7F75C3C8F3288D7F75C3",
    "0x519221726F9C2239AFAECDA0AA2239AFAECDA0AA",
    "0x574ABEE83CD184C74DE5AA05F884C74DE5AA05F8",
    "0x974CA75B37E25AF6A97CEF83555AF6A97CEF8355",
    "0xC238C5F9AC8B178DCB71CC13C0178DCB71CC13C0",
    "0x15AC4B2F5AA895F80302C2442B95F80302C2442B",
    "0x4B6489E181688892382C88520B8892382C88520B",
    "0xF50DE302A58ACEFCA6F09D3954CEFCA6F09D3954",
    "0x04EE3FA0B0FEA6E08B9453025CA6E08B9453025C",
    "0x34ED2D4C49402A51A5A6AC55382A51A5A6AC5538",
    "0x07DBAD027F7590D46F50D7217790D46F50D72177",
    "0x535F73A1D383737C8D23E5E754737C8D23E5E754",
    "0x2816FA7B55A6B5456B0D4E1DB0B5456B0D4E1DB0",
    "0xE03BF40F564F583F42789DFE9C583F42789DFE9C",
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

  func addAccount(
    coin: BraveWallet.CoinType,
    keyringId: BraveWallet.KeyringId,
    accountName: String,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    let address = nextAddress()
    let info = BraveWallet.AccountInfo(
      accountId: .init(
        coin: coin,
        keyringId: keyringId,
        kind: .imported,
        address: address,
        accountIndex: 0,
        uniqueKey: address
      ),
      address: address,
      name: accountName,
      hardware: nil
    )
    allAccounts.append(info)
    observers.allObjects.forEach {
      $0.accountsChanged()
    }
    completion(info)
  }

  func createWallet(password: String, completion: @escaping (String?) -> Void) {
    isLocked = false
    self.password = password
    addAccount(
      coin: .eth,
      keyringId: BraveWallet.KeyringId.default,
      accountName: "Account 1"
    ) { [self] accountInfo in
      selectedAccount = accountInfo
      observers.allObjects.forEach {
        $0.walletCreated()
      }
      completion(mnemonic)
    }
  }

  func isLocked(completion: @escaping (Bool) -> Void) {
    completion(isLocked)
  }

  func lock() {
    isLocked = true
    observers.allObjects.forEach {
      $0.locked()
    }
  }

  func isWalletBackedUp(completion: @escaping (Bool) -> Void) {
    completion(isBackedUp)
  }

  func walletMnemonic(password: String, completion: @escaping (String?) -> Void) {
    completion(mnemonic)
  }

  func unlock(password: String, completion: @escaping (Bool) -> Void) {
    if !isWalletCreated {
      completion(false)
      return
    }
    let passwordsMatch = self.password == password
    if passwordsMatch {
      isLocked = false
      observers.allObjects.forEach {
        $0.unlocked()
      }
    }
    completion(passwordsMatch)
  }

  func notifyWalletBackupComplete() {
    isBackedUp = true
    observers.allObjects.forEach {
      $0.backedUp()
    }
  }

  func restoreWallet(
    mnemonic: String,
    password: String,
    isLegacyEthSeedFormat: Bool,
    completion: @escaping (Bool) -> Void
  ) {
    self.password = password
    // Test store does not test phrase validity
    observers.allObjects.forEach {
      $0.walletRestored()
    }
    completion(true)
  }

  func validatePassword(_ password: String, completion: @escaping (Bool) -> Void) {
    completion(password == self.password)
  }

  // To ensure previews are consistent, use the same set of addresses per run
  private var importedTestAddresses = [
    "0xB05618F6B379B38B120D8EB4E5B38B120D8EB4E5",
    "0xDFF7D1EDDE1484D547E8F41BEE84D547E8F41BEE",
    "0xB1D205B163D1C6681FD327AE33C6681FD327AE33",
    "0x4C71547BC5433B950EDB336A923B950EDB336A92",
    "0x6FE03430626DEB934B1E13EA51EB934B1E13EA51",
    "0xCC625A5A6511392FA1D40BD970392FA1D40BD970",
    "0x701E8FEDAEE87F868A4899D40D7F868A4899D40D",
    "0x5F95775E02A89F70FF8A81B33C9F70FF8A81B33C",
    "0x4DADD3FC345C0B7BA559B757F50B7BA559B757F5",
    "0xAAE5BD4958DD443F85B0F3D606443F85B0F3D606",
    "0xF6F9D52299CA8F94FCC0C4BDC48F94FCC0C4BDC4",
    "0xD07C121B7372159875E2B7DE46159875E2B7DE46",
    "0xF1E1EFDE452626CD0FE670328426CD0FE6703284",
    "0x1F58637A1FD7B71BCBEC8F90A8B71BCBEC8F90A8",
    "0xF854F9456C013B8F75F48D42CE3B8F75F48D42CE",
    "0xF6CD2E28C549CB0B04F33AB5A5CB0B04F33AB5A5",
    "0xF0CA976FE1A503C88C1E626FC103C88C1E626FC1",
    "0x03B8680DF0B949B7D265177F8E49B7D265177F8E",
    "0x8C0033DEDD3E4A02B5FD22DB1A4A02B5FD22DB1A",
    "0x41B59E6D21216C8FBE3C4C4EAC6C8FBE3C4C4EAC",
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

  func importAccountFromJson(
    accountName: String,
    privateKey: String,
    coin: BraveWallet.CoinType,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    let address = nextImportedAddress()
    let info = BraveWallet.AccountInfo(
      accountId: .init(
        coin: .eth,
        keyringId: BraveWallet.KeyringId.default,
        kind: .imported,
        address: address,
        accountIndex: 0,
        uniqueKey: address
      ),
      address: address,
      name: accountName,
      hardware: nil
    )
    privateKeys[info.address] = privateKey
    allAccounts.append(info)
    observers.allObjects.forEach {
      $0.accountsChanged()
    }
    completion(info)
  }

  func importEthereumAccountFromJson(
    accountName: String,
    password: String,
    json: String,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    completion(nil)
  }
  func importFilecoinAccount(
    accountName: String,
    privateKey: String,
    network: String,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    completion(nil)
  }

  func importBitcoinAccount(
    accountName: String,
    payload: String,
    network: String,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    completion(nil)
  }

  func importEthereumAccount(
    accountName: String,
    privateKey: String,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    completion(nil)
  }

  func importSolanaAccount(
    accountName: String,
    privateKey: String,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    completion(nil)
  }

  func encodePrivateKeyForExport(
    accountId: BraveWallet.AccountId,
    password: String,
    completion: @escaping (String?) -> Void
  ) {
    completion("807df4db569fab37cdf475a4bda779897f0f3dd9c5d90a2cb953c88ef762fd96")
  }

  func privateKey(
    forImportedAccount address: String,
    coin: BraveWallet.CoinType,
    completion: @escaping (Bool, String) -> Void
  ) {
    if let key = privateKeys[address] {
      completion(true, key)
    } else {
      completion(false, "")
    }
  }

  func removeAccount(
    accountId: BraveWallet.AccountId,
    password: String,
    completion: @escaping (Bool) -> Void
  ) {
    guard let index = allAccounts.firstIndex(where: { $0.address == accountId.address }) else {
      completion(false)
      return
    }
    allAccounts.remove(at: index)
    observers.allObjects.forEach {
      $0.accountsChanged()
    }
    completion(true)
  }

  func setDefaultKeyringHardwareAccountName(
    _ address: String,
    name: String,
    completion: @escaping (Bool) -> Void
  ) {
    // Hardware wallets not supported on iOS
    completion(false)
  }

  func hardwareAccounts(_ completion: @escaping ([BraveWallet.AccountInfo]) -> Void) {
    // Hardware wallets not supported on iOS
    completion([])
  }

  func selectedAccount(_ coin: BraveWallet.CoinType, completion: @escaping (String?) -> Void) {
    completion(selectedAccount?.address)
  }

  func setSelectedAccount(accountId: BraveWallet.AccountId, completion: @escaping (Bool) -> Void) {
    guard let account = allAccounts.first(where: { $0.address == accountId.address }) else {
      completion(false)
      return
    }
    selectedAccount = account
    completion(true)
  }

  private var autoLockMinutes: Int32 = 5

  func autoLockMinutes(completion: @escaping (Int32) -> Void) {
    completion(autoLockMinutes)
  }

  func setAutoLockMinutes(_ minutes: Int32, completion: @escaping (Bool) -> Void) {
    autoLockMinutes = minutes
    completion(true)
  }

  func isStrongPassword(_ password: String, completion: @escaping (Bool) -> Void) {
    completion(password.count >= 8)
  }

  func checksumEthAddress(_ address: String, completion: @escaping (String) -> Void) {
    completion("")
  }

  func hasPendingUnlockRequest(completion: @escaping (Bool) -> Void) {
    completion(false)
  }

  func setAccountName(
    accountId: BraveWallet.AccountId,
    name: String,
    completion: @escaping (Bool) -> Void
  ) {
    guard let account = allAccounts.first(where: { $0.address == accountId.address }) else {
      completion(false)
      return
    }
    account.name = name
    completion(true)
  }

  func addHardwareAccounts(
    info: [BraveWallet.HardwareWalletAccount]
  ) async -> [BraveWallet.AccountInfo]? {
    nil
  }

  func addBitcoinHardwareAccount(
    info: BraveWallet.HardwareWalletAccount,
    completion: @escaping (Bool) -> Void
  ) {
    completion(true)
  }

  func setHardwareAccountName(
    _ coin: BraveWallet.CoinType,
    keyringId: String,
    address: String,
    name: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(true)
  }

  func removeHardwareAccount(
    _ coin: BraveWallet.CoinType,
    keyringId: String,
    address: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(true)
  }

  func notifyUserInteraction() {
  }

  func addFilecoinAccount(
    _ accountName: String,
    filecoinNetwork fileCoinNetwork: String,
    completion: @escaping (Bool) -> Void
  ) {
    completion(false)
  }

  func filecoinSelectedAccount(_ network: String, completion: @escaping (String?) -> Void) {
    completion("")
  }

  func addBitcoinAccount(
    _ accountName: String,
    networkId: String,
    keyringId: BraveWallet.KeyringId,
    completion: @escaping (BraveWallet.AccountInfo?) -> Void
  ) {
    let bitcoinAccount = BraveWallet.AccountInfo(
      accountId: .init(
        coin: .btc,
        keyringId: keyringId,
        kind: .derived,
        address: "btc1",
        accountIndex: 0,
        uniqueKey: "btc1"
      ),
      address: "btc1",
      name: "BTC Account 1",
      hardware: nil
    )
    completion(bitcoinAccount)
  }

  func allAccounts(completion: @escaping (BraveWallet.AllAccountsInfo) -> Void) {

  }

  func isWalletCreated(completion: @escaping (Bool) -> Void) {
    completion(isWalletCreated)
  }
}

extension BraveWallet.AccountInfo {
  static let mockEthAccount: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .eth,
      keyringId: BraveWallet.KeyringId.default,
      kind: .derived,
      address: "mock_eth_id",
      accountIndex: 0,
      uniqueKey: "mock_eth_id"
    ),
    address: "mock_eth_id",
    name: "Ethereum Account 1",
    hardware: nil
  )

  static let mockSolAccount: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .sol,
      keyringId: BraveWallet.KeyringId.solana,
      kind: .derived,
      address: "mock_sol_id",
      accountIndex: 0,
      uniqueKey: "mock_sol_id"
    ),
    address: "mock_sol_id",
    name: "Solana Account 1",
    hardware: nil
  )

  static let mockFilAccount: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .fil,
      keyringId: BraveWallet.KeyringId.filecoin,
      kind: .derived,
      address: "mock_fil_id",
      accountIndex: 0,
      uniqueKey: "mock_fil_id"
    ),
    address: "mock_fil_id",
    name: "Filecoin Account 1",
    hardware: nil
  )

  static let mockFilTestnetAccount: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .fil,
      keyringId: BraveWallet.KeyringId.filecoinTestnet,
      kind: .derived,
      address: "mock_fil_testnet_id",
      accountIndex: 0,
      uniqueKey: "mock_fil_testnet_id"
    ),
    address: "mock_fil_testnet_id",
    name: "Filecoin Testnet 1",
    hardware: nil
  )

  static let mockBtcAccount: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .btc,
      keyringId: BraveWallet.KeyringId.bitcoin84,
      kind: .derived,
      address: "",
      accountIndex: 0,
      uniqueKey: "4_0_0_0"
    ),
    address: "",
    name: "Bitcoin Account 1",
    hardware: nil
  )

  static let mockBtcTestnetAccount: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .btc,
      keyringId: BraveWallet.KeyringId.bitcoin84Testnet,
      kind: .derived,
      address: "",
      accountIndex: 0,
      uniqueKey: "5_0_0_0"
    ),
    address: "",
    name: "Bitcoin Testnet 1",
    hardware: nil
  )
}

extension BraveWallet.AllAccountsInfo {
  static let mock: BraveWallet.AllAccountsInfo = .init(
    accounts: [
      .mockEthAccount,
      .mockSolAccount,
      .mockFilAccount,
      .mockFilTestnetAccount,
      .mockBtcAccount,
      .mockBtcTestnetAccount,
    ],
    selectedAccount: .mockEthAccount,
    ethDappSelectedAccount: .mockEthAccount,
    solDappSelectedAccount: .mockSolAccount
  )
}
