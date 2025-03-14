// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

protocol TabObserver: AnyObject {
  func tab(_ tab: Tab, didAddSnackbar bar: SnackBar)
  func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar)
  /// Triggered when "Search with Brave" is selected on selected web text
  func tab(_ tab: Tab, didSelectSearchWithBraveFor selectedText: String)
  func tab(_ tab: Tab, didCreateWebView webView: UIView)
  func tab(_ tab: Tab, willDeleteWebView webView: UIView)

  func tabDidStartNavigation(_ tab: Tab)
  func tabDidCommitNavigation(_ tab: Tab)
  func tabDidFinishNavigation(_ tab: Tab)
  func tab(_ tab: Tab, didFailNavigationWithError error: Error)

  func tabDidUpdateURL(_ tab: Tab)
  func tabDidChangeTitle(_ tab: Tab)
  func tabDidStartLoading(_ tab: Tab)
  func tabDidStopLoading(_ tab: Tab)
  func tabDidChangeLoadProgress(_ tab: Tab)
  func tabDidChangeVisibleSecurityState(_ tab: Tab)
  func tabDidChangeBackForwardState(_ tab: Tab)
  func tabDidChangeSampledPageTopColor(_ tab: Tab)

  /// Called when the Tab is about to deinit, use this to remove any observers/policy deciders added
  /// to the Tab.
  ///
  /// - warning: The supplied `tab` will nil immediately after execution, therefore you cannot
  ///            capture this tab in escaping closures
  func tabWillBeDestroyed(_ tab: Tab)
}

extension TabObserver {
  func tab(_ tab: Tab, didAddSnackbar bar: SnackBar) {}
  func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar) {}
  func tab(_ tab: Tab, didSelectSearchWithBraveFor selectedText: String) {}
  func tab(_ tab: Tab, didCreateWebView webView: UIView) {}
  func tab(_ tab: Tab, willDeleteWebView webView: UIView) {}

  func tabDidStartNavigation(_ tab: Tab) {}
  func tabDidCommitNavigation(_ tab: Tab) {}
  func tabDidFinishNavigation(_ tab: Tab) {}
  func tab(_ tab: Tab, didFailNavigationWithError error: Error) {}

  func tabDidUpdateURL(_ tab: Tab) {}
  func tabDidChangeTitle(_ tab: Tab) {}
  func tabDidStartLoading(_ tab: Tab) {}
  func tabDidStopLoading(_ tab: Tab) {}
  func tabDidChangeLoadProgress(_ tab: Tab) {}
  func tabDidChangeVisibleSecurityState(_ tab: Tab) {}
  func tabDidChangeBackForwardState(_ tab: Tab) {}
  func tabDidChangeSampledPageTopColor(_ tab: Tab) {}
  func tabWillBeDestroyed(_ tab: Tab) {}
}

class AnyTabObserver: TabObserver, Hashable {
  let id: ObjectIdentifier
  private let _tabDidAddSnackbar: (Tab, SnackBar) -> Void
  private let _tabDidRemoveSnackbar: (Tab, SnackBar) -> Void
  private let _tabDidSelectSearchWithBraveFor: (Tab, String) -> Void
  private let _tabDidCreateWebView: (Tab, UIView) -> Void
  private let _tabWillDeleteWebView: (Tab, UIView) -> Void

  private let _tabDidStartNavigation: (Tab) -> Void
  private let _tabDidCommitNavigation: (Tab) -> Void
  private let _tabDidFinishNavigation: (Tab) -> Void
  private let _tabDidFailNavigationWithError: (Tab, Error) -> Void

  private let _tabDidUpdateURL: (Tab) -> Void
  private let _tabDidChangeTitle: (Tab) -> Void
  private let _tabDidStartLoading: (Tab) -> Void
  private let _tabDidStopLoading: (Tab) -> Void
  private let _tabDidChangeLoadProgress: (Tab) -> Void
  private let _tabDidChangeVisibleSecurityState: (Tab) -> Void
  private let _tabDidChangeBackForwardState: (Tab) -> Void
  private let _tabDidChangeSampledPageTopColor: (Tab) -> Void
  private let _tabWillBeDestroyed: (Tab) -> Void

  func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }

  static func == (lhs: AnyTabObserver, rhs: AnyTabObserver) -> Bool {
    lhs.id == rhs.id
  }

  init(_ observer: some TabObserver) {
    id = ObjectIdentifier(observer)
    _tabDidAddSnackbar = { [weak observer] in observer?.tab($0, didAddSnackbar: $1) }
    _tabDidRemoveSnackbar = { [weak observer] in observer?.tab($0, didRemoveSnackbar: $1) }
    _tabDidSelectSearchWithBraveFor = { [weak observer] in
      observer?.tab($0, didSelectSearchWithBraveFor: $1)
    }
    _tabDidCreateWebView = { [weak observer] in observer?.tab($0, didCreateWebView: $1) }
    _tabWillDeleteWebView = { [weak observer] in observer?.tab($0, willDeleteWebView: $1) }
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

  func tab(_ tab: Tab, didAddSnackbar bar: SnackBar) {
    _tabDidAddSnackbar(tab, bar)
  }
  func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar) {
    _tabDidRemoveSnackbar(tab, bar)
  }
  func tab(_ tab: Tab, didSelectSearchWithBraveFor selectedText: String) {
    _tabDidSelectSearchWithBraveFor(tab, selectedText)
  }
  func tab(_ tab: Tab, didCreateWebView webView: UIView) {
    _tabDidCreateWebView(tab, webView)
  }
  func tab(_ tab: Tab, willDeleteWebView webView: UIView) {
    _tabWillDeleteWebView(tab, webView)
  }
  func tabDidStartNavigation(_ tab: Tab) {
    _tabDidStartNavigation(tab)
  }
  func tabDidCommitNavigation(_ tab: Tab) {
    _tabDidCommitNavigation(tab)
  }
  func tabDidFinishNavigation(_ tab: Tab) {
    _tabDidFinishNavigation(tab)
  }
  func tab(_ tab: Tab, didFailNavigationWithError error: Error) {
    _tabDidFailNavigationWithError(tab, error)
  }
  func tabDidUpdateURL(_ tab: Tab) {
    _tabDidUpdateURL(tab)
  }
  func tabDidChangeTitle(_ tab: Tab) {
    _tabDidChangeTitle(tab)
  }
  func tabDidStartLoading(_ tab: Tab) {
    _tabDidStartLoading(tab)
  }
  func tabDidStopLoading(_ tab: Tab) {
    _tabDidStopLoading(tab)
  }
  func tabDidChangeLoadProgress(_ tab: Tab) {
    _tabDidChangeLoadProgress(tab)
  }
  func tabDidChangeVisibleSecurityState(_ tab: Tab) {
    _tabDidChangeVisibleSecurityState(tab)
  }
  func tabDidChangeBackForwardState(_ tab: Tab) {
    _tabDidChangeBackForwardState(tab)
  }
  func tabDidChangeSampledPageTopColor(_ tab: Tab) {
    _tabDidChangeSampledPageTopColor(tab)
  }
  func tabWillBeDestroyed(_ tab: Tab) {
    _tabWillBeDestroyed(tab)
  }
}
