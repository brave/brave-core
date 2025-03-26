// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreSpotlight
import Foundation
import MobileCoreServices
import Shared
import Storage
import Web
import WebKit

private let browsingActivityType: String = "com.brave.ios.browsing"

private let searchableIndex = CSSearchableIndex(name: "firefox")

class UserActivityHandler {
  private var tabObservers: TabObservers!

  init() {
    self.tabObservers = registerFor(
      .didLoseFocus,
      .didGainFocus,
      .didChangeURL,
      .didLoadPageMetadata,
      // .didLoadFavicon, // TODO: Bug 1390294
      .didClose,
      queue: .main
    )
  }

  deinit {
    unregister(tabObservers)
  }

  class func clearSearchIndex(completionHandler: ((Error?) -> Void)? = nil) {
    searchableIndex.deleteAllSearchableItems(completionHandler: completionHandler)
  }

  fileprivate func setUserActivityForTab(_ tab: TabState, url: URL) {
    guard !tab.isPrivate, url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url)
    else {
      tab.userActivity?.resignCurrent()
      tab.userActivity = nil
      return
    }

    tab.userActivity?.invalidate()

    let userActivity = NSUserActivity(activityType: browsingActivityType)
    userActivity.webpageURL = url
    userActivity.becomeCurrent()

    tab.userActivity = userActivity
  }
}

extension UserActivityHandler: TabEventHandler {
  func tabDidGainFocus(_ tab: TabState) {
    tab.userActivity?.becomeCurrent()
  }

  func tabDidLoseFocus(_ tab: TabState) {
    tab.userActivity?.resignCurrent()
  }

  func tab(_ tab: TabState, didChangeURL url: URL) {
    setUserActivityForTab(tab, url: url)
  }

  func tab(_ tab: TabState, didLoadPageMetadata metadata: PageMetadata) {
    guard let url = URL(string: metadata.siteURL) else {
      return
    }

    setUserActivityForTab(tab, url: url)
  }

  func tabDidClose(_ tab: TabState) {
    guard let userActivity = tab.userActivity else {
      return
    }
    tab.userActivity = nil
    userActivity.invalidate()
  }
}
