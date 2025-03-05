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
  func tab(_ tab: Tab, didCreateWebView webView: WKWebView)
  func tab(_ tab: Tab, willDeleteWebView webView: WKWebView)

  func tabDidUpdateURL(_ tab: Tab)
  func tabDidChangeTitle(_ tab: Tab)
  func tabDidStartLoading(_ tab: Tab)
  func tabDidStopLoading(_ tab: Tab)
  func tabDidChangeLoadProgress(_ tab: Tab)
  func tabDidChangeVisibleSecurityState(_ tab: Tab)
  func tabDidChangeBackForwardState(_ tab: Tab)
  func tabDidChangeSampledPageTopColor(_ tab: Tab)
}

extension TabObserver {
  func tab(_ tab: Tab, didAddSnackbar bar: SnackBar) {}
  func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar) {}
  func tab(_ tab: Tab, didSelectSearchWithBraveFor selectedText: String) {}
  func tab(_ tab: Tab, didCreateWebView webView: WKWebView) {}
  func tab(_ tab: Tab, willDeleteWebView webView: WKWebView) {}

  func tabDidUpdateURL(_ tab: Tab) {}
  func tabDidChangeTitle(_ tab: Tab) {}
  func tabDidStartLoading(_ tab: Tab) {}
  func tabDidStopLoading(_ tab: Tab) {}
  func tabDidChangeLoadProgress(_ tab: Tab) {}
  func tabDidChangeVisibleSecurityState(_ tab: Tab) {}
  func tabDidChangeBackForwardState(_ tab: Tab) {}
  func tabDidChangeSampledPageTopColor(_ tab: Tab) {}
}

class AnyTabObserver: TabObserver, Hashable {
  let id: ObjectIdentifier
  private let _tabDidAddSnackbar: (Tab, SnackBar) -> Void
  private let _tabDidRemoveSnackbar: (Tab, SnackBar) -> Void
  private let _tabDidSelectSearchWithBraveFor: (Tab, String) -> Void
  private let _tabDidCreateWebView: (Tab, WKWebView) -> Void
  private let _tabWillDeleteWebView: (Tab, WKWebView) -> Void

  private let _tabDidUpdateURL: (Tab) -> Void
  private let _tabDidChangeTitle: (Tab) -> Void
  private let _tabDidStartLoading: (Tab) -> Void
  private let _tabDidStopLoading: (Tab) -> Void
  private let _tabDidChangeLoadProgress: (Tab) -> Void
  private let _tabDidChangeVisibleSecurityState: (Tab) -> Void
  private let _tabDidChangeBackForwardState: (Tab) -> Void
  private let _tabDidChangeSampledPageTopColor: (Tab) -> Void

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
  func tab(_ tab: Tab, didCreateWebView webView: WKWebView) {
    _tabDidCreateWebView(tab, webView)
  }
  func tab(_ tab: Tab, willDeleteWebView webView: WKWebView) {
    _tabWillDeleteWebView(tab, webView)
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
}
