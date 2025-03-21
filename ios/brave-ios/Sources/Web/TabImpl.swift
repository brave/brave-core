// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

protocol TabImpl: Tab {
  /* weak */ var delegate: TabDelegate? { get set }
  /* weak */ var downloadDelegate: TabDownloadDelegate? { get set }
  var observers: Set<AnyTabObserver> { get set }
  var policyDeciders: Set<AnyTabPolicyDecider> { get set }

  func shouldAllowRequest(
    _ request: URLRequest,
    requestInfo: WebRequestInfo
  ) async -> WebPolicyDecision

  func shouldAllowResponse(
    _ response: URLResponse,
    responseInfo: WebResponseInfo
  ) async -> WebPolicyDecision

  func didStartNavigation()

  func didCommitNavigation()

  func didFinishNavigation()

  func didFailNavigation(with error: Error)
}

extension TabImpl {
  func addPolicyDecider(_ policyDecider: some TabPolicyDecider) {
    policyDeciders.insert(.init(policyDecider))
  }

  func removePolicyDecider(_ policyDecider: some TabPolicyDecider) {
    if let policyDecider = policyDeciders.first(where: { $0.id == ObjectIdentifier(policyDecider) })
    {
      policyDeciders.remove(policyDecider)
    }
  }

  func addObserver(_ observer: some TabObserver) {
    observers.insert(.init(observer))
  }

  func removeObserver(_ observer: some TabObserver) {
    if let observer = observers.first(where: { $0.id == ObjectIdentifier(observer) }) {
      observers.remove(observer)
    }
  }

  private func resolvedPolicyDecision(
    for policyDeciders: Set<AnyTabPolicyDecider>,
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
