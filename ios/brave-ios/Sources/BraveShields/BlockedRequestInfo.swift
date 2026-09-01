// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import Foundation

public struct BlockedRequestInfo: Hashable, Identifiable {
  public enum Location: String {
    case contentBlocker
    case requestBlocking

    var display: String {
      switch self {
      case .contentBlocker:
        return Strings.Shields.contentBlocker
      case .requestBlocking:
        return Strings.Shields.requestBlocking
      }
    }
  }
  public let requestURL: URL
  public let sourceURL: URL
  public let resourceType: AdblockResourceType
  public let isAggressive: Bool
  public let location: Location

  public var id: String {
    "\(requestURL)\(sourceURL)\(resourceType.rawValue)\(isAggressive)\(location.rawValue)"
  }

  public init(
    requestURL: URL,
    sourceURL: URL,
    resourceType: AdblockResourceType,
    isAggressive: Bool,
    location: Location
  ) {
    self.requestURL = requestURL
    self.sourceURL = sourceURL
    self.resourceType = resourceType
    self.isAggressive = isAggressive
    self.location = location
  }
}
