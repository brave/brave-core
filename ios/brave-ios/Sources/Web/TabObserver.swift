// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

public protocol TabObserver: AnyObject {
  func tabDidCreateWebView(_ tab: some TabState)
  func tabWillDeleteWebView(_ tab: some TabState)

  func tabWasShown(_ tab: some TabState)
  func tabWasHidden(_ tab: some TabState)

  func tabDidStartNavigation(_ tab: some TabState)
  func tabDidCommitNavigation(_ tab: some TabState)
  func tabDidRedirectNavigation(_ tab: some TabState)
  func tabDidFinishNavigation(_ tab: some TabState)
  func tab(_ tab: some TabState, didFailNavigationWithError error: Error)

  func tabDidUpdateURL(_ tab: some TabState)
  func tabDidChangeTitle(_ tab: some TabState)
  func tabDidStartLoading(_ tab: some TabState)
  func tabDidStopLoading(_ tab: some TabState)
  func tabDidChangeLoadProgress(_ tab: some TabState)
  func tabDidChangeVisibleSecurityState(_ tab: some TabState)
  func tabDidChangeBackForwardState(_ tab: some TabState)
  func tabDidChangeSampledPageTopColor(_ tab: some TabState)

  /// Called when the Tab is about to deinit, use this to remove any observers/policy deciders added
  /// to the Tab.
  ///
  /// - warning: The supplied `tab` will nil immediately after execution, therefore you cannot
  ///            capture this tab in escaping closures
  func tabWillBeDestroyed(_ tab: some TabState)
}

extension TabObserver {
  public func tabDidCreateWebView(_ tab: some TabState) {}
  public func tabWillDeleteWebView(_ tab: some TabState) {}

  public func tabWasShown(_ tab: some TabState) {}
  public func tabWasHidden(_ tab: some TabState) {}

  public func tabDidStartNavigation(_ tab: some TabState) {}
  public func tabDidCommitNavigation(_ tab: some TabState) {}
  public func tabDidRedirectNavigation(_ tab: some TabState) {}
  public func tabDidFinishNavigation(_ tab: some TabState) {}
  public func tab(_ tab: some TabState, didFailNavigationWithError error: Error) {}

  public func tabDidUpdateURL(_ tab: some TabState) {}
  public func tabDidChangeTitle(_ tab: some TabState) {}
  public func tabDidStartLoading(_ tab: some TabState) {}
  public func tabDidStopLoading(_ tab: some TabState) {}
  public func tabDidChangeLoadProgress(_ tab: some TabState) {}
  public func tabDidChangeVisibleSecurityState(_ tab: some TabState) {}
  public func tabDidChangeBackForwardState(_ tab: some TabState) {}
  public func tabDidChangeSampledPageTopColor(_ tab: some TabState) {}
  public func tabWillBeDestroyed(_ tab: some TabState) {}
}

class AnyTabObserver: TabObserver, Hashable, CustomDebugStringConvertible {
  let id: ObjectIdentifier
  #if DEBUG
  let objectName: String
  #endif
  private let _tabDidCreateWebView: (any TabState) -> Void
  private let _tabWillDeleteWebView: (any TabState) -> Void

  private let _tabWasShown: (any TabState) -> Void
  private let _tabWasHidden: (any TabState) -> Void

  private let _tabDidStartNavigation: (any TabState) -> Void
  private let _tabDidCommitNavigation: (any TabState) -> Void
  private let _tabDidRedirectNavigation: (any TabState) -> Void
  private let _tabDidFinishNavigation: (any TabState) -> Void
  private let _tabDidFailNavigationWithError: (any TabState, Error) -> Void

  private let _tabDidUpdateURL: (any TabState) -> Void
  private let _tabDidChangeTitle: (any TabState) -> Void
  private let _tabDidStartLoading: (any TabState) -> Void
  private let _tabDidStopLoading: (any TabState) -> Void
  private let _tabDidChangeLoadProgress: (any TabState) -> Void
  private let _tabDidChangeVisibleSecurityState: (any TabState) -> Void
  private let _tabDidChangeBackForwardState: (any TabState) -> Void
  private let _tabDidChangeSampledPageTopColor: (any TabState) -> Void
  private let _tabWillBeDestroyed: (any TabState) -> Void

  var debugDescription: String {
    #if DEBUG
    return "AnyTabObserver: \(objectName)"
    #else
    return "AnyTabObserver: \(id)"
    #endif
  }

  func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }

  static func == (lhs: AnyTabObserver, rhs: AnyTabObserver) -> Bool {
    lhs.id == rhs.id
  }

  init(_ observer: some TabObserver) {
    id = ObjectIdentifier(observer)
    #if DEBUG
    objectName = String(describing: observer)
    #endif
    _tabDidCreateWebView = { [weak observer] in observer?.tabDidCreateWebView($0) }
    _tabWillDeleteWebView = { [weak observer] in observer?.tabWillDeleteWebView($0) }
    _tabWasShown = { [weak observer] in observer?.tabWasShown($0) }
    _tabWasHidden = { [weak observer] in observer?.tabWasHidden($0) }
    _tabDidStartNavigation = { [weak observer] in observer?.tabDidStartNavigation($0) }
    _tabDidCommitNavigation = { [weak observer] in observer?.tabDidCommitNavigation($0) }
    _tabDidRedirectNavigation = { [weak observer] in observer?.tabDidRedirectNavigation($0) }
    _tabDidFinishNavigation = { [weak observer] in observer?.tabDidFinishNavigation($0) }
    _tabDidFailNavigationWithError = { [weak observer] in
      observer?.tab($0, didFailNavigationWithError: $1)
    }
    _tabDidUpdateURL = { [weak observer] in observer?.tabDidUpdateURL($0) }
    _tabDidChangeTitle = { [weak observer] in observer?.tabDidChangeTitle($0) }
    _tabDidStartLoading = { [weak observer] in observer?.tabDidStartLoading($0) }
    _tabDidStopLoading = { [weak observer] in observer?.tabDidStopLoading($0) }
    _tabDidChangeLoadProgress = { [weak observer] in observer?.tabDidChangeLoadProgress($0) }
    _tabDidChangeVisibleSecurityState = { [weak observer] in
      observer?.tabDidChangeVisibleSecurityState($0)
    }
    _tabDidChangeBackForwardState = { [weak observer] in observer?.tabDidChangeBackForwardState($0)
    }
    _tabDidChangeSampledPageTopColor = { [weak observer] in
      observer?.tabDidChangeSampledPageTopColor($0)
    }
    _tabWillBeDestroyed = { [weak observer] in observer?.tabWillBeDestroyed($0) }
  }

  func tabDidCreateWebView(_ tab: some TabState) {
    _tabDidCreateWebView(tab)
  }
  func tabWillDeleteWebView(_ tab: some TabState) {
    _tabWillDeleteWebView(tab)
  }
  func tabWasShown(_ tab: some TabState) {
    _tabWasShown(tab)
  }
  func tabWasHidden(_ tab: some TabState) {
    _tabWasHidden(tab)
  }
  func tabDidStartNavigation(_ tab: some TabState) {
    _tabDidStartNavigation(tab)
  }
  func tabDidCommitNavigation(_ tab: some TabState) {
    _tabDidCommitNavigation(tab)
  }
  func tabDidRedirectNavigation(_ tab: some TabState) {
    _tabDidRedirectNavigation(tab)
  }
  func tabDidFinishNavigation(_ tab: some TabState) {
    _tabDidFinishNavigation(tab)
  }
  func tab(_ tab: some TabState, didFailNavigationWithError error: Error) {
    _tabDidFailNavigationWithError(tab, error)
  }
  func tabDidUpdateURL(_ tab: some TabState) {
    _tabDidUpdateURL(tab)
  }
  func tabDidChangeTitle(_ tab: some TabState) {
    _tabDidChangeTitle(tab)
  }
  func tabDidStartLoading(_ tab: some TabState) {
    _tabDidStartLoading(tab)
  }
  func tabDidStopLoading(_ tab: some TabState) {
    _tabDidStopLoading(tab)
  }
  func tabDidChangeLoadProgress(_ tab: some TabState) {
    _tabDidChangeLoadProgress(tab)
  }
  func tabDidChangeVisibleSecurityState(_ tab: some TabState) {
    _tabDidChangeVisibleSecurityState(tab)
  }
  func tabDidChangeBackForwardState(_ tab: some TabState) {
    _tabDidChangeBackForwardState(tab)
  }
  func tabDidChangeSampledPageTopColor(_ tab: some TabState) {
    _tabDidChangeSampledPageTopColor(tab)
  }
  func tabWillBeDestroyed(_ tab: some TabState) {
    _tabWillBeDestroyed(tab)
  }
}
