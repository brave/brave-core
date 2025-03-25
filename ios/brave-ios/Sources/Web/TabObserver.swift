// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

public protocol TabObserver: AnyObject {
  func tab(_ tab: TabState, didCreateWebView webView: UIView)
  func tab(_ tab: TabState, willDeleteWebView webView: UIView)

  func tabWasShown(_ tab: TabState)
  func tabWasHidden(_ tab: TabState)

  func tabDidStartNavigation(_ tab: TabState)
  func tabDidCommitNavigation(_ tab: TabState)
  func tabDidFinishNavigation(_ tab: TabState)
  func tab(_ tab: TabState, didFailNavigationWithError error: Error)

  func tabDidUpdateURL(_ tab: TabState)
  func tabDidChangeTitle(_ tab: TabState)
  func tabDidStartLoading(_ tab: TabState)
  func tabDidStopLoading(_ tab: TabState)
  func tabDidChangeLoadProgress(_ tab: TabState)
  func tabDidChangeVisibleSecurityState(_ tab: TabState)
  func tabDidChangeBackForwardState(_ tab: TabState)
  func tabDidChangeSampledPageTopColor(_ tab: TabState)

  /// Called when the Tab is about to deinit, use this to remove any observers/policy deciders added
  /// to the Tab.
  ///
  /// - warning: The supplied `tab` will nil immediately after execution, therefore you cannot
  ///            capture this tab in escaping closures
  func tabWillBeDestroyed(_ tab: TabState)
}

extension TabObserver {
  public func tab(_ tab: TabState, didCreateWebView webView: UIView) {}
  public func tab(_ tab: TabState, willDeleteWebView webView: UIView) {}

  public func tabWasShown(_ tab: TabState) {}
  public func tabWasHidden(_ tab: TabState) {}

  public func tabDidStartNavigation(_ tab: TabState) {}
  public func tabDidCommitNavigation(_ tab: TabState) {}
  public func tabDidFinishNavigation(_ tab: TabState) {}
  public func tab(_ tab: TabState, didFailNavigationWithError error: Error) {}

  public func tabDidUpdateURL(_ tab: TabState) {}
  public func tabDidChangeTitle(_ tab: TabState) {}
  public func tabDidStartLoading(_ tab: TabState) {}
  public func tabDidStopLoading(_ tab: TabState) {}
  public func tabDidChangeLoadProgress(_ tab: TabState) {}
  public func tabDidChangeVisibleSecurityState(_ tab: TabState) {}
  public func tabDidChangeBackForwardState(_ tab: TabState) {}
  public func tabDidChangeSampledPageTopColor(_ tab: TabState) {}
  public func tabWillBeDestroyed(_ tab: TabState) {}
}

class AnyTabObserver: TabObserver, Hashable {
  let id: ObjectIdentifier
  private let _tabDidCreateWebView: (TabState, UIView) -> Void
  private let _tabWillDeleteWebView: (TabState, UIView) -> Void

  private let _tabWasShown: (TabState) -> Void
  private let _tabWasHidden: (TabState) -> Void

  private let _tabDidStartNavigation: (TabState) -> Void
  private let _tabDidCommitNavigation: (TabState) -> Void
  private let _tabDidFinishNavigation: (TabState) -> Void
  private let _tabDidFailNavigationWithError: (TabState, Error) -> Void

  private let _tabDidUpdateURL: (TabState) -> Void
  private let _tabDidChangeTitle: (TabState) -> Void
  private let _tabDidStartLoading: (TabState) -> Void
  private let _tabDidStopLoading: (TabState) -> Void
  private let _tabDidChangeLoadProgress: (TabState) -> Void
  private let _tabDidChangeVisibleSecurityState: (TabState) -> Void
  private let _tabDidChangeBackForwardState: (TabState) -> Void
  private let _tabDidChangeSampledPageTopColor: (TabState) -> Void
  private let _tabWillBeDestroyed: (TabState) -> Void

  func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }

  static func == (lhs: AnyTabObserver, rhs: AnyTabObserver) -> Bool {
    lhs.id == rhs.id
  }

  init(_ observer: some TabObserver) {
    id = ObjectIdentifier(observer)
    _tabDidCreateWebView = { [weak observer] in observer?.tab($0, didCreateWebView: $1) }
    _tabWillDeleteWebView = { [weak observer] in observer?.tab($0, willDeleteWebView: $1) }
    _tabWasShown = { [weak observer] in observer?.tabWasShown($0) }
    _tabWasHidden = { [weak observer] in observer?.tabWasHidden($0) }
    _tabDidStartNavigation = { [weak observer] in observer?.tabDidStartNavigation($0) }
    _tabDidCommitNavigation = { [weak observer] in observer?.tabDidCommitNavigation($0) }
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

  func tab(_ tab: TabState, didCreateWebView webView: UIView) {
    _tabDidCreateWebView(tab, webView)
  }
  func tab(_ tab: TabState, willDeleteWebView webView: UIView) {
    _tabWillDeleteWebView(tab, webView)
  }
  func tabWasShown(_ tab: TabState) {
    _tabWasShown(tab)
  }
  func tabWasHidden(_ tab: TabState) {
    _tabWasHidden(tab)
  }
  func tabDidStartNavigation(_ tab: TabState) {
    _tabDidStartNavigation(tab)
  }
  func tabDidCommitNavigation(_ tab: TabState) {
    _tabDidCommitNavigation(tab)
  }
  func tabDidFinishNavigation(_ tab: TabState) {
    _tabDidFinishNavigation(tab)
  }
  func tab(_ tab: TabState, didFailNavigationWithError error: Error) {
    _tabDidFailNavigationWithError(tab, error)
  }
  func tabDidUpdateURL(_ tab: TabState) {
    _tabDidUpdateURL(tab)
  }
  func tabDidChangeTitle(_ tab: TabState) {
    _tabDidChangeTitle(tab)
  }
  func tabDidStartLoading(_ tab: TabState) {
    _tabDidStartLoading(tab)
  }
  func tabDidStopLoading(_ tab: TabState) {
    _tabDidStopLoading(tab)
  }
  func tabDidChangeLoadProgress(_ tab: TabState) {
    _tabDidChangeLoadProgress(tab)
  }
  func tabDidChangeVisibleSecurityState(_ tab: TabState) {
    _tabDidChangeVisibleSecurityState(tab)
  }
  func tabDidChangeBackForwardState(_ tab: TabState) {
    _tabDidChangeBackForwardState(tab)
  }
  func tabDidChangeSampledPageTopColor(_ tab: TabState) {
    _tabDidChangeSampledPageTopColor(tab)
  }
  func tabWillBeDestroyed(_ tab: TabState) {
    _tabWillBeDestroyed(tab)
  }
}
