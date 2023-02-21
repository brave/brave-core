// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CommonCrypto

/// An error class representing an error that has occurred when handling encryption
public struct CryptographyError: Error {
  // The error domain
  public let domain: String

  // The error code
  public let code: Int32

  // A description of the error
  public let description: String?

  init(code: Int32 = -1, description: String? = nil) {
    self.domain = "com.brave.security.cryptography.error"
    self.code = code
    self.description = description
  }
}

/// A class representing a cryptographic key.
public struct CryptographicKey {
  private let key: SecKey
  private let keyId: String

  public init(key: SecKey, keyId: String) {
    self.key = key
    self.keyId = keyId
  }

  /// Returns the public key's SHA-256 fingerprint hex encoded
  public func getPublicKeySha256FingerPrint() throws -> String? {
    guard let data = try getPublicKeyAsDER() else {
      return nil
    }

    var hash = [UInt8](repeating: 0, count: Int(CC_SHA256_DIGEST_LENGTH))
    _ = data.withUnsafeBytes { CC_SHA256($0.baseAddress, CC_LONG(data.count), &hash) }
    return Data(hash).map({ String(format: "%02x", UInt8($0)) }).joined()
  }

  /// Returns the public key in PEM format (full ASN.1 DER header encoded)
  public func getPublicAsPEM() throws -> String? {
    guard let data = try getPublicKeyAsDER() else {
      return nil
    }

    let result =
      """
      -----BEGIN PUBLIC KEY-----
      \(data.base64EncodedString(options: [.lineLength64Characters, .endLineWithLineFeed]))
      -----END PUBLIC KEY-----
      """
    return result
  }

  /// Deletes the key from the secure-enclave and keychain
  @discardableResult
  public func delete() -> Error? {
    return Cryptography.delete(id: keyId)
  }

  /// Signs "message" with the key and returns the signature
  public func sign(message: Data) throws -> Data {
    var error: Unmanaged<CFError>?
    let signature = SecKeyCreateSignature(
      key,
      .ecdsaSignatureMessageX962SHA256,
      message as CFData,
      &error)

    if let error = error?.takeUnretainedValue() {
      throw error as Error
    }

    guard let result = signature as Data? else {
      throw CryptographyError(description: "Cannot sign message with cryptographic key.")
    }

    return result
  }

  /// Signs a "message" with the key and returns the signature
  public func sign(message: String) throws -> Data {
    guard let message = message.data(using: .utf8) else {
      throw CryptographyError(description: "Cannot Sign Message: Invalid Message")
    }

    return try sign(message: message)
  }

  /// Verifies the signature of "message" with the public key
  public func verify(message: Data, signature: Data) throws -> Bool {
    guard let publicKey = getPublicKey() else {
      throw CryptographyError(description: "Cannot retrieve public key")
    }

    var error: Unmanaged<CFError>?
    let result = SecKeyVerifySignature(publicKey, .ecdsaSignatureMessageX962SHA256, message as CFData, signature as CFData, &error)

    if let error = error?.takeUnretainedValue() {
      throw error as Error
    }

    return result
  }

  /// Returns the public key in ASN.1 DER format with the full 30-byte TAG header encoding
  private func getPublicKeyAsDER() throws -> Data? {
    guard let publicKeyRepresentation = try getPublicKeyExternalRepresentation() else {
      return nil
    }

    // opensource.apple.com/source/security_certtool/security_certtool-55103/src/dumpasn1.cfg
    // OID = 06 07 2A 86 48 CE 3D 02 01
    // Comment = ANSI X9.62 public key type
    // Description = ecPublicKey (1 2 840 10045 2 1)
    let curveOIDHeader: [UInt8] = [0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00]

    var data = Data(bytes: curveOIDHeader, count: curveOIDHeader.count)
    data.append(publicKeyRepresentation)
    return data
  }

  /// Returns the private key
  private func getPrivateKey() -> SecKey {
    return key
  }

  /// Returns the public key
  private func getPublicKey() -> SecKey? {
    return SecKeyCopyPublicKey(key)
  }

  /// Returns the public key in ASN.1 format
  private func getPublicKeyExternalRepresentation() throws -> Data? {
    guard let publicKey = getPublicKey() else {
      throw CryptographyError(description: "Cannot retrieve public key")
    }

    var error: Unmanaged<CFError>?
    if let data = SecKeyCopyExternalRepresentation(publicKey, &error) {
      return data as Data
    }

    if let error = error?.takeUnretainedValue() {
      throw error
    }

    return nil
  }
}

/// A class used for generating cryptographic keys
public class Cryptography {

  /// The access control flags for any keys generated
  public static let accessControlFlags = SecAccessControlCreateWithFlags(kCFAllocatorDefault, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, [.privateKeyUsage], nil)  // .biometryAny

  /// Determines if a key exists in the keychain without retrieving it
  public class func keyExists(id: String) -> Bool {
    let properties = getKeyProperties(id: id)
    return properties.status == errSecSuccess || properties.status == errSecInteractionNotAllowed
  }

  /// Determines if a key requires biometrics to access
  public class func isKeyRequiringBiometrics(id: String) -> Bool {
    let properties = getKeyProperties(id: id)
    if properties.status == errSecSuccess || properties.status == errSecInteractionNotAllowed {
      if let result = properties.result as? [String: Any],
        let item = result[kSecAttrAccessControl as String] as CFTypeRef?,
        CFGetTypeID(item) == SecAccessControlGetTypeID() {

        let accessControl = item as! SecAccessControl  // swiftlint:disable:this force_cast
        return String(describing: accessControl).contains("bio")
      }
    }

    return false
  }

  /// Retrieves an existing key from the secure-enclave
  public class func getExistingKey(id: String) throws -> CryptographicKey? {
    guard let keyId = id.data(using: .utf8) else {
      throw CryptographyError(description: "Invalid Key Id")
    }

    let query: [CFString: Any] = [
      kSecClass: kSecClassKey,
      kSecAttrApplicationTag: keyId,
      kSecMatchLimit: kSecMatchLimitOne,
      kSecReturnRef: kCFBooleanTrue as Any,
    ]

    var result: CFTypeRef?
    let error = SecItemCopyMatching(query as CFDictionary, &result)
    if error == errSecSuccess || error == errSecDuplicateItem || error == errSecInteractionNotAllowed {
      if let result = result, CFGetTypeID(result) == SecKeyGetTypeID() {
        return CryptographicKey(key: result as! SecKey, keyId: id)  // swiftlint:disable:this force_cast
      }
      return nil
    }

    if error == errSecItemNotFound {
      return nil
    }

    throw CryptographyError(code: error)
  }

  /// Generates a new key and stores it in the secure-enclave
  /// If a key with the specified ID already exists, it retrieves the existing key instead
  /// The generated key is a 256-bit RSA ECSEC Key.
  public class func generateKey(
    id: String,
    bits: UInt16 = 256,
    storeInKeychain: Bool = true,
    secureEnclave: Bool = true,
    controlFlags: SecAccessControl? = Cryptography.accessControlFlags
  ) throws -> CryptographicKey? {

    if let key = try getExistingKey(id: id) {
      return key
    }

    guard let keyId = id.data(using: .utf8) else {
      throw CryptographyError(description: "Invalid Key Id")
    }

    let attributes: [CFString: Any] = [
      kSecClass: kSecClassKey,
      kSecAttrKeyType: kSecAttrKeyTypeECSECPrimeRandom,
      kSecAttrKeySizeInBits: bits,
      kSecAttrCreator: "com.brave.security.cryptography",
      kSecAttrTokenID: (secureEnclave ? kSecAttrTokenIDSecureEnclave : nil) as Any,
      kSecPrivateKeyAttrs: [
        kSecAttrIsPermanent: storeInKeychain,
        kSecAttrApplicationTag: keyId,
        kSecAttrAccessControl: (controlFlags ?? nil) as Any,
      ],
    ]

    var error: Unmanaged<CFError>?
    let key = SecKeyCreateRandomKey(attributes as CFDictionary, &error)

    if let error = error?.takeUnretainedValue() {
      throw error as Error
    }

    guard let pKey = key else {
      throw CryptographyError(description: "Cannot generate cryptographic key.")
    }

    return CryptographicKey(key: pKey, keyId: id)
  }

  /// Deletes the key with the specified ID from the secure-enclave and keychain
  @discardableResult
  public class func delete(id: String) -> Error? {
    guard let keyId = id.data(using: .utf8) else {
      return CryptographyError(description: "Invalid KeyId")
    }

    let query: [CFString: Any] = [
      kSecClass: kSecClassKey,
      kSecAttrApplicationTag: keyId,
    ]
    let error = SecItemDelete(query as CFDictionary)

    if error == errSecSuccess || error == errSecItemNotFound {
      return nil
    }

    return CryptographyError(code: error)
  }

  /// Retrieve a key's properties without retrieving the actual key itself
  private class func getKeyProperties(id: String) -> (status: OSStatus, result: CFTypeRef?) {
    guard let keyId = id.data(using: .utf8) else {
      return (errSecParam, nil)
    }

    let query: [CFString: Any] = [
      kSecClass: kSecClassKey,
      kSecAttrApplicationTag: keyId,
      kSecMatchLimit: kSecMatchLimitOne,
      kSecReturnRef: kCFBooleanFalse as Any,
      kSecReturnAttributes: kCFBooleanTrue as Any,
    ]

    var result: CFTypeRef?
    let error = SecItemCopyMatching(query as CFDictionary, &result)
    return (error, result)
  }
}
