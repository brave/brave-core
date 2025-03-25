// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Decides the navigation policy for a tab's navigations
public protocol TabPolicyDecider: AnyObject {
  /// Decide whether or not a request should be allowed
  func tab(
    _ tab: Tab,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision

  /// Decide whether or not a response should be allowed
  func tab(
    _ tab: Tab,
    shouldAllowResponse response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision
}

extension TabPolicyDecider {
  public func tab(
    _ tab: Tab,
    shouldAllowRequest: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    return .allow
  }

  public func tab(
    _ tab: Tab,
    shouldAllowResponse: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    return .allow
  }
}

class AnyTabPolicyDecider: TabPolicyDecider, Hashable {
  var id: ObjectIdentifier

  private let _shouldAllowRequest: (Tab, URLRequest, WebRequestInfo) async -> WebPolicyDecision
  private let _shouldAllowResponse: (Tab, URLResponse, WebResponseInfo) async -> WebPolicyDecision

  init(_ policyDecider: some TabPolicyDecider) {
    id = ObjectIdentifier(policyDecider)
    _shouldAllowRequest = { [weak policyDecider] in
      await policyDecider?.tab($0, shouldAllowRequest: $1, requestInfo: $2) ?? .allow
    }
    _shouldAllowResponse = { [weak policyDecider] in
      await policyDecider?.tab($0, shouldAllowResponse: $1, responseInfo: $2) ?? .allow
    }
  }

  func tab(
    _ tab: Tab,
    shouldAllowRequest request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    return await _shouldAllowRequest(tab, request, requestInfo)
  }

  func tab(
    _ tab: Tab,
    shouldAllowResponse response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    return await _shouldAllowResponse(tab, response, responseInfo)
  }

  static func == (lhs: AnyTabPolicyDecider, rhs: AnyTabPolicyDecider) -> Bool {
    lhs.id == rhs.id
  }

  func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }
}

/// The policy to pass back to a policy decider
public enum WebPolicyDecision {
  case allow
  case cancel
}

/// The type of action triggering a navigation
public enum WebNavigationType: Int {
  case linkActivated
  case formSubmitted
  case backForward
  case reload
  case formResubmitted
  case other = -1
}

/// Information about an action that may trigger a navigation, which can be used to make policy
/// decisions.
public struct WebRequestInfo {
  public var navigationType: WebNavigationType
  public var isMainFrame: Bool
  public var isNewWindow: Bool
  public var isUserInitiated: Bool
}

/// Information about a navigation response that can be used to make policy decisions
public struct WebResponseInfo {
  public var isForMainFrame: Bool
}
