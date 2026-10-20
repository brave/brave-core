// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import Foundation
import Security

/// Generic passwords in the app group's keychain access group, so the app and
/// the share extension see the same items. An app group identifier is a valid
/// keychain access group without a `keychain-access-groups` entitlement.
/// Items are this-device-only and readable after first unlock.
public final class TransferKeychain {
  public static let service = "com.brave.transfer"

  private let accessGroup: String

  public init(accessGroup: String) {
    self.accessGroup = accessGroup
  }

  private func query(_ account: String) -> [String: Any] {
    [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrService as String: Self.service,
      kSecAttrAccount as String: account,
      kSecAttrAccessGroup as String: accessGroup,
      kSecAttrSynchronizable as String: kCFBooleanFalse as Any,
      kSecUseDataProtectionKeychain as String: true,
    ]
  }

  public func data(for account: String) -> Data? {
    var query = query(account)
    query[kSecReturnData as String] = true
    query[kSecMatchLimit as String] = kSecMatchLimitOne
    var result: CFTypeRef?
    guard SecItemCopyMatching(query as CFDictionary, &result) == errSecSuccess else {
      return nil
    }
    return result as? Data
  }

  @discardableResult
  public func set(_ data: Data, for account: String) -> Bool {
    let status = SecItemUpdate(
      query(account) as CFDictionary,
      [kSecValueData as String: data] as CFDictionary
    )
    if status == errSecSuccess {
      return true
    }
    guard status == errSecItemNotFound else { return false }
    var attributes = query(account)
    attributes[kSecValueData as String] = data
    attributes[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly
    return SecItemAdd(attributes as CFDictionary, nil) == errSecSuccess
  }

  @discardableResult
  public func remove(_ account: String) -> Bool {
    let status = SecItemDelete(query(account) as CFDictionary)
    return status == errSecSuccess || status == errSecItemNotFound
  }
}

/// This device's keys (design plan §3.2). The static key is the device's
/// long-term identity, shared by the app and the extension so a desktop that
/// pinned it trusts both. The root key is exported by the app while Sync is
/// active and removed when it is not.
public final class TransferKeyStore {
  static let staticKeyAccount = "static-private-key"
  static let rootKeyAccount = "chain-root-key"

  private let keychain: TransferKeychain

  public init(keychain: TransferKeychain) {
    self.keychain = keychain
  }

  /// The X25519 static private key, generated on first use.
  public func staticPrivateKey() -> Bytes? {
    if let data = keychain.data(for: Self.staticKeyAccount), data.count == 32 {
      return Bytes(data)
    }
    let key = StaticKey.generatePrivateKey()
    guard keychain.set(Data(key), for: Self.staticKeyAccount) else { return nil }
    return key
  }

  public var rootKey: Bytes? {
    guard let data = keychain.data(for: Self.rootKeyAccount), data.count == 32 else {
      return nil
    }
    return Bytes(data)
  }

  /// Stores `rootKey`, or removes it when nil. Returns whether it changed.
  @discardableResult
  public func setRootKey(_ rootKey: Bytes?) -> Bool {
    guard rootKey != self.rootKey else { return false }
    if let rootKey {
      keychain.set(Data(rootKey), for: Self.rootKeyAccount)
    } else {
      keychain.remove(Self.rootKeyAccount)
    }
    return true
  }
}
