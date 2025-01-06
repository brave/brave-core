// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import DesignSystem
import Foundation
import Shared
import UserAgent
import WebKit

class BraveWebView: WKWebView {
  /// Stores last position when the webview was touched on.
  private(set) var lastHitPoint = CGPoint(x: 0, y: 0)

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

    super.init(frame: frame, configuration: configuration)

    isFindInteractionEnabled = true

    customUserAgent = UserAgent.userAgentForIdiom()
    if #available(iOS 16.4, *) {
      isInspectable = true
    }

    backgroundColor = UIColor(braveSystemName: .containerBackground)
    scrollView.backgroundColor = UIColor(braveSystemName: .containerBackground)
    // WKWebView flashes white screen on load regardless of background colour assignments without
    // setting `isOpaque` to false
    isOpaque = false
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

extension WKWebView {
  public var sessionData: Data? {
    get {
      interactionState as? Data
    }
    set {
      interactionState = newValue
    }
  }
}
