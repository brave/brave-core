// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import OrderedCollections

protocol TabStateImpl: TabState {
  var observers: OrderedSet<AnyTabObserver> { get set }
  var policyDeciders: OrderedSet<AnyTabPolicyDecider> { get set }

  func shouldAllowRequest(
    _ request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision

  func shouldAllowResponse(
    _ response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision

  func didCreateWebView()

  func didStartNavigation()

  func didCommitNavigation()

  func didRedirectNavigation()

  func didFinishNavigation()

  func didFailNavigation(with error: Error)
}

extension TabStateImpl {
  func addPolicyDecider(_ policyDecider: some TabPolicyDecider) {
    policyDeciders.append(.init(policyDecider))
  }

  func removePolicyDecider(_ policyDecider: some TabPolicyDecider) {
    if let policyDecider = policyDeciders.first(where: { $0.id == ObjectIdentifier(policyDecider) })
    {
      policyDeciders.remove(policyDecider)
    }
  }

  func addObserver(_ observer: some TabObserver) {
    observers.append(.init(observer))
  }

  func removeObserver(_ observer: some TabObserver) {
    if let observer = observers.first(where: { $0.id == ObjectIdentifier(observer) }) {
      observers.remove(observer)
    }
  }

  private func resolvedPolicyDecision(
    for policyDeciders: OrderedSet<AnyTabPolicyDecider>,
    task: @escaping (AnyTabPolicyDecider) async -> WebPolicyDecision
  ) async -> WebPolicyDecision {
    let result = await withTaskGroup(of: WebPolicyDecision.self, returning: WebPolicyDecision.self)
    { group in
      for policyDecider in policyDeciders {
        group.addTask { @MainActor in
          if Task.isCancelled {
            return .cancel
          }
          let decision = await task(policyDecider)
          if decision == .cancel {
            return .cancel
          }
          return .allow
        }
      }
      for await result in group {
        if result == .cancel {
          group.cancelAll()
          return .cancel
        }
      }
      return .allow
    }
    return result
  }

  func shouldAllowRequest(
    _ request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision {
    return await resolvedPolicyDecision(for: policyDeciders) { policyDecider in
      await policyDecider.tab(
        self,
        shouldAllowRequest: request,
        requestInfo: requestInfo
      )
    }
  }

  func shouldAllowResponse(
    _ response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision {
    return await resolvedPolicyDecision(for: policyDeciders) { policyDecider in
      await policyDecider.tab(
        self,
        shouldAllowResponse: response,
        responseInfo: responseInfo
      )
    }
  }

  func didCreateWebView() {
    // Make sure to remove any message handlers on newly created web views
    configuration.userContentController.removeAllScriptMessageHandlers()
    observers.forEach {
      $0.tabDidCreateWebView(self)
    }
  }

  func didStartNavigation() {
    observers.forEach {
      $0.tabDidStartNavigation(self)
    }
  }

  func didCommitNavigation() {
    observers.forEach {
      $0.tabDidCommitNavigation(self)
    }
  }

  func didRedirectNavigation() {
    observers.forEach {
      $0.tabDidRedirectNavigation(self)
    }
  }

  func didFinishNavigation() {
    observers.forEach {
      $0.tabDidFinishNavigation(self)
    }
  }

  func didFailNavigation(with error: Error) {
    observers.forEach {
      $0.tab(self, didFailNavigationWithError: error)
    }
  }
}
