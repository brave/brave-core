// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A helper structure to deal with universal link handling.
public struct UniversalLinkManager {
  public enum LinkType: CaseIterable {
    // https://vpn.brave.com/.well-known/apple-app-site-association
    case buyVPN

    public var associatedDomain: String {
      switch self {
      case .buyVPN: return "vpn.brave.com"
      }
    }

    public var path: String {
      switch self {
      case .buyVPN: return "/dl/"
      }
    }
  }

  /// Returns a universal link type for given URL. Returns nil if the URL is not a universal link
  ///
  /// - parameter url: URL to to check against.
  /// - parameter checkPath: If false, only url host is checked, if true we check for both host and path matching.
  /// This is because we have to handle universal link from 2 places. When a user opens it from another app, path checking is not needed
  /// since this is handled by the `AppDelegate`. If the link is opened from the Brave app itself and it matches the associated domain
  /// app delegate doesn't handle such case, it must be handled manually.
  public static func universalLinkType(for url: URL, checkPath: Bool) -> LinkType? {
    guard let components = URLComponents(url: url, resolvingAgainstBaseURL: false),
      let host = components.host
    else { return nil }

    if checkPath {
      for type in LinkType.allCases where type.associatedDomain == host {
        if components.path.starts(with: type.path) { return type }
      }
    } else {
      return LinkType.allCases.first(where: { $0.associatedDomain == host })
    }

    return nil
  }
}
