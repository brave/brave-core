// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CryptoKit

/// Class that aids creating the same random values for any given eTLD.
///
/// Constructing this class the same eTLD+1 will result in the same random values
/// (provided the same `sessionKey` is used)
/// - Note: Any string can actually be used and this class can be used for more general purpose applications
class RandomConfiguration {
  /// This is used to encode the domain key (i.e. the eTLD+1).
  ///
  /// For farbling, this key should be the same for the lifecycle of the app. Hence we use a static variable.
  private static let sessionKey = SymmetricKey(size: .bits256)

  /// The eTLD+1 this random manager was created with
  private let etld: String

  /// The session key this random manager was created with
  private let sessionKey: SymmetricKey

  /// The 64-character hex domain key for the provided eTLD+1
  ///
  /// - Note: A 64 character hex is represented by 256 bits. This means we can use it directly in our `ARC4RandomNumberGenerator`.
  private(set) lazy var domainKeyData: Data = {
    let signature = HMAC<SHA256>.authenticationCode(for: Data(etld.utf8), using: sessionKey)
    return Data(signature)
  }()

  /// The domain key as hex `String`
  private(set) lazy var domainKeyHEX: String = {
    domainKeyData.hexString
  }()

  /// The domain key as a `SymmetricKey`
  private(set) lazy var domainKey: SymmetricKey = {
    return SymmetricKey(data: domainKeyData)
  }()

  /// Initialize this class with an eTLD+1 and a sessionKey.
  ///
  /// If no sessionKey is provided, a shared session key will be used.
  /// The shared session key is lost when the application terminates.
  /// - Note: This is what we want for farbling.
  init(etld: String, sessionKey: SymmetricKey = RandomConfiguration.sessionKey) {
    self.etld = etld
    self.sessionKey = sessionKey
  }

  /// Signs this given value using the `domainKey`
  func domainSignedKey(for value: String) -> String {
    let signature = HMAC<SHA256>.authenticationCode(for: Data(value.utf8), using: domainKey)
    return Data(signature).hexString
  }
}

private extension Data {
  var hexString: String {
    map({ String(format: "%02hhx", $0) }).joined()
  }
}
