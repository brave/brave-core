// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension AdblockEngine {
  public enum ResourceType: String, Decodable {
    case xmlhttprequest, script, image
  }
  
  /// Check the rust engine if the request should be blocked given the `sourceURL` and `resourceType`.
  ///
  /// - Warning: You must provide a absolute URL (i.e. containing a host) fo r `requestURL` and `sourceURL`
  public func shouldBlock(requestURL: URL, sourceURL: URL, resourceType: ResourceType) -> Bool {
    // Compare the etld+1 of requestURL and sourceURL.
    // Note: `baseDomain` returns etld+1
    let isThirdParty = requestURL.baseDomain != sourceURL.baseDomain
    
    guard requestURL.scheme != "data" else {
      // TODO: @JS Investigate if we need to deal with data schemes and if so, how?
      return false
    }
    
    guard sourceURL.absoluteString != "about:blank" else {
      // TODO: @JS Investigate why sometimes `sourceURL` is `about:blank` and find out how to deal with it
      return false
    }
    
    guard let requestHost = requestURL.host, let sourceHost = sourceURL.host else {
      return false
    }
    
    return matches(
      url: requestURL.absoluteString,
      host: requestHost,
      tabHost: sourceHost,
      isThirdParty: isThirdParty,
      resourceType: resourceType.rawValue
    ).didMatchRule
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
