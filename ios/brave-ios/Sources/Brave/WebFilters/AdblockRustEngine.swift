// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Foundation

/// The outcome of checking a request against one or more adblock engines.
public struct AdblockRequestResult: Equatable, Sendable {
  public var shouldBlock: Bool
  /// A `data:` URL with replacement content for the blocked resource.
  public var redirect: String?
  /// The request URL with query params removed by `$removeparam` rules.
  public var rewrittenURL: URL?

  public static let none = AdblockRequestResult(shouldBlock: false)

  public init(shouldBlock: Bool, redirect: String? = nil, rewrittenURL: URL? = nil) {
    self.shouldBlock = shouldBlock
    self.redirect = redirect
    self.rewrittenURL = rewrittenURL
  }
}

extension AdblockEngine {
  /// Check the rust engine if the request should be blocked given the `sourceURL` and `resourceType`.
  ///
  /// - Warning: You must provide a absolute URL (i.e. containing a host) fo r `requestURL` and `sourceURL`
  public func shouldBlock(
    requestURL: URL,
    sourceURL: URL,
    resourceType: AdblockResourceType,
    isAggressive: Bool
  ) -> Bool {
    requestResult(
      requestURL: requestURL,
      sourceURL: sourceURL,
      resourceType: resourceType,
      isAggressive: isAggressive
    ).shouldBlock
  }

  /// Check the rust engine for a block decision along with any redirect resource or rewritten URL.
  ///
  /// - Warning: You must provide a absolute URL (i.e. containing a host) for `requestURL` and `sourceURL`
  public func requestResult(
    requestURL: URL,
    sourceURL: URL,
    resourceType: AdblockResourceType,
    isAggressive: Bool
  ) -> AdblockRequestResult {
    guard requestURL.scheme != "data" else {
      // TODO: @JS Investigate if we need to deal with data schemes and if so, how?
      return .none
    }

    guard let requestDomain = requestURL.baseDomain, let sourceDomain = sourceURL.baseDomain
    else {
      return .none
    }

    guard let requestHost = requestURL.host, let sourceHost = sourceURL.host else {
      return .none
    }

    // The content blocker rule for third party is the following:
    // "third-party triggers if the resource isn’t from the same domain as the main page resource"
    let isThirdParty = requestDomain != sourceDomain

    if !isAggressive {
      // If we have standard mode for this engine,
      // we don't block first party ads
      guard isThirdParty else {
        return .none
      }
    }

    let result = matches(
      url: requestURL.absoluteString,
      host: requestHost,
      tabHost: sourceHost,
      isThirdParty: isThirdParty,
      resourceType: resourceType.rawValue
    )
    return AdblockRequestResult(
      shouldBlock: result.didMatchRule,
      redirect: result.didMatchRule ? result.redirect.flatMap { $0.isEmpty ? nil : $0 } : nil,
      rewrittenURL: result.rewrittenURL.flatMap { $0.isEmpty ? nil : URL(string: $0) }
    )
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
