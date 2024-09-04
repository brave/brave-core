// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension AdblockEngine {
  public enum ResourceType: String, Decodable {
    case xmlhttprequest
    case script
    case image
    case document
    case subdocument
  }

  /// Check the rust engine if the request should be blocked given the `sourceURL` and `resourceType`.
  ///
  /// - Warning: You must provide a absolute URL (i.e. containing a host) fo r `requestURL` and `sourceURL`
  public func shouldBlock(
    requestURL: URL,
    sourceURL: URL,
    resourceType: ResourceType,
    isAggressive: Bool
  ) -> Bool {
    guard requestURL.scheme != "data" else {
      // TODO: @JS Investigate if we need to deal with data schemes and if so, how?
      return false
    }

    guard let requestDomain = requestURL.baseDomain, let sourceDomain = sourceURL.baseDomain else {
      return false
    }

    guard let requestHost = requestURL.host, let sourceHost = sourceURL.host else {
      return false
    }

    // The content blocker rule for third party is the following:
    // "third-party triggers if the resource isn’t from the same domain as the main page resource"
    let isThirdParty = requestDomain != sourceDomain

    if !isAggressive {
      // If we have standard mode for this engine,
      // we don't block first party ads
      guard isThirdParty else {
        return false
      }
    }

    let matchesResult = matches(
      url: requestURL.absoluteString,
      host: requestHost,
      tabHost: sourceHost,
      isThirdParty: isThirdParty,
      resourceType: resourceType.rawValue
    )

    if matchesResult.didMatchRule, let filter = matchesResult.filter {
      print("Blocking `\(requestURL.absoluteString)` due to rule `\(filter)`")
    }
    return matchesResult.didMatchRule
  }

  @available(*, deprecated, renamed: "deserialize(data:)")
  @discardableResult
  public func set(data: Data) -> Bool {
    deserialize(data: data)
  }

  @available(*, deprecated, message: "Use AdblockEngine.addResources(_:)")
  public func set(json: Data) -> Bool {
    guard let string = String(data: json, encoding: .utf8) else {
      return false
    }
    useResources(string)
    return true
  }
}

@available(*, deprecated, renamed: "AdblockEngine")
typealias AdblockRustEngine = AdblockEngine
