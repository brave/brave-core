// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Shared
import UserAgent
import WebKit

class BraveWebView: CWVWebView {
  /// Stores last position when the webview was touched on.
  private(set) var lastHitPoint = CGPoint(x: 0, y: 0)

  var underlyingWebView: WKWebView?

  private static var nonPersistentDataStore: WKWebsiteDataStore?
  static func sharedNonPersistentStore() -> WKWebsiteDataStore {
    if let dataStore = nonPersistentDataStore {
      return dataStore
    }

    let dataStore = WKWebsiteDataStore.nonPersistent()
    nonPersistentDataStore = dataStore
    return dataStore
  }

  init(
    frame: CGRect,
    configuration: WKWebViewConfiguration = WKWebViewConfiguration(),
    isPrivate: Bool = true
  ) {
    if isPrivate {
      configuration.websiteDataStore = BraveWebView.sharedNonPersistentStore()
    } else {
      configuration.websiteDataStore = WKWebsiteDataStore.default()
    }
    CWVWebView.webInspectorEnabled = true
    CWVWebView.customUserAgent = UserAgent.userAgentForDesktopMode
    CWVWebView.chromeContextMenuEnabled = false
    CWVWebView.skipAccountStorageCheckEnabled = true

    super.init(
      frame: frame,
      configuration: isPrivate ? .incognito() : .default(),
      wkConfiguration: configuration,
      createdWKWebView: &underlyingWebView
    )

    underlyingWebView?.isFindInteractionEnabled = true
  }

  static func removeNonPersistentStore() {
    BraveWebView.nonPersistentDataStore = nil
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    lastHitPoint = point
    return super.hitTest(point, with: event)
  }
}

extension CWVWebView {
  public var sessionData: Data? {
    if lastCommittedURL == nil { return nil }
    let coder = NSKeyedArchiver(requiringSecureCoding: false)
    encodeRestorableState(with: coder)
    return coder.encodedData
  }
}
