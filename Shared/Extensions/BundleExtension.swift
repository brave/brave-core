// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension Bundle {
  public static let shared: Bundle = Bundle(identifier: "com.brave.Shared")!
  public static let data: Bundle = Bundle(identifier: "com.brave.Data")!
  public static let braveShared: Bundle = Bundle(identifier: "com.brave.BraveShared")!
  public static let braveWallet: Bundle = Bundle(identifier: "com.brave.BraveWallet")!
  public static let storage: Bundle = Bundle(identifier: "com.brave.Storage")!

  public func getPlistString(for key: String) -> String? {
    self.infoDictionary?[key] as? String
  }
}

extension Bundle {
  /// Get a string from the info plist dictionary for a given key
  public func infoDictionaryString(forKey key: String) -> String {
    guard let value = object(forInfoDictionaryKey: key) as? String else {
      if AppConstants.isRunningTest {
        /// Using dummy info dictionary while running tests, without host app set.
        return "com.brave.tests"
      }
      fatalError("Could not find a string value in the Info dictionary with the key: \(key)")
    }
    return value
  }
}
