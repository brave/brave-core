// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import LocalAuthentication
import Security

public protocol KeychainType: AnyObject {
  func storePasswordInKeychain(key: String, password: String) -> OSStatus
  @discardableResult func resetPasswordInKeychain(key: String) -> Bool
  func isPasswordStoredInKeychain(key: String) -> Bool
  func getPasswordFromKeychain(key: String) -> String?
}

public class Keychain: KeychainType {

  public init() {}

  public func storePasswordInKeychain(key: String, password: String) -> OSStatus {
    guard let passwordData = password.data(using: .utf8) else { return errSecInvalidData }
    #if targetEnvironment(simulator)
    // There is a bug with iOS 15 simulators when attempting to add a keychain item with
    // `kSecAttrAccessControl` set. This of course means that on simulator we will not ask for biometrics
    // and it will just auto-fill the password field but at least to set it up you still need to enable
    // biometrics on the simulator
    //
    // Last checked: Xcode 13.1 (13A1030d)
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: key,
      kSecValueData as String: passwordData,
    ]
    #else
    let accessControl = SecAccessControlCreateWithFlags(
      nil,
      kSecAttrAccessibleWhenPasscodeSetThisDeviceOnly,
      .userPresence,
      nil
    )
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: key,
      kSecAttrAccessControl as String: accessControl as Any,
      kSecValueData as String: passwordData,
    ]
    #endif
    return SecItemAdd(query as CFDictionary, nil)
  }

  @discardableResult
  public func resetPasswordInKeychain(key: String) -> Bool {
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: key,
    ]
    let status = SecItemDelete(query as CFDictionary)
    return status == errSecSuccess
  }

  public func isPasswordStoredInKeychain(key: String) -> Bool {
    let context = LAContext()
    context.interactionNotAllowed = true
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: key,
      kSecMatchLimit as String: kSecMatchLimitOne,
      kSecUseAuthenticationContext as String: context,
    ]
    let status = SecItemCopyMatching(query as CFDictionary, nil)
    #if targetEnvironment(simulator)
    // See comment in `storePasswordInKeychain(_:)`
    return status == errSecSuccess
    #else
    return status == errSecInteractionNotAllowed
    #endif
  }

  public func getPasswordFromKeychain(key: String) -> String? {
    let query: [String: Any] = [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrAccount as String: key,
      kSecMatchLimit as String: kSecMatchLimitOne,
      kSecReturnData as String: true,
    ]
    var passwordData: AnyObject?
    let status = SecItemCopyMatching(query as CFDictionary, &passwordData)
    guard status == errSecSuccess,
      let data = passwordData as? Data,
      let password = String(data: data, encoding: .utf8)
    else {
      return nil
    }
    return password
  }
}

#if DEBUG
public class TestableKeychain: KeychainType {
  public var _storePasswordInKeychain: ((_ key: String, _ password: String) -> OSStatus)?
  public var _resetPasswordInKeychain: ((_ key: String) -> Bool)?
  public var _isPasswordStoredInKeychain: ((_ key: String) -> Bool)?
  public var _getPasswordFromKeychain: ((_ key: String) -> String?)?

  public init() {}

  public func storePasswordInKeychain(key: String, password: String) -> OSStatus {
    _storePasswordInKeychain?(key, password) ?? OSStatus(0)
  }

  public func resetPasswordInKeychain(key: String) -> Bool {
    _resetPasswordInKeychain?(key) ?? false
  }

  public func isPasswordStoredInKeychain(key: String) -> Bool {
    _isPasswordStoredInKeychain?(key) ?? false
  }

  public func getPasswordFromKeychain(key: String) -> String? {
    _getPasswordFromKeychain?(key)
  }
}
#endif
