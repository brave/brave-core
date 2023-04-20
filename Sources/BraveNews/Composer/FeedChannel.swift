// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A locale-specific channel in Brave News
///
/// Each region & language combination will have its own set of channels, and sometimes they will overlap
/// with other regions (i.e. "Top Sources" which will be available in each locale)
public struct FeedChannel: Hashable, Identifiable {
  /// The locale identifier associated with the channel (i.e. `en_US`)
  public var localeIdentifier: String
  /// The name of the channel
  public var name: String
  
  public init(localeIdentifier: String, name: String) {
    self.localeIdentifier = localeIdentifier
    self.name = name
  }
  
  public var id: String {
    "\(localeIdentifier)_\(name)"
  }
  
  /// A human readable description of the locale when showing multiple of the same Channel in one list.
  ///
  /// For example, if the `localeIdentiifer` was `en_CA`, this would return `Canada`
  public var localeDescription: String? {
    let locale = Locale(identifier: localeIdentifier)
    guard let regionCode = locale.regionCode else { return nil }
    return Locale.current.localizedString(forRegionCode: regionCode)
  }
}
