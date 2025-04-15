// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreSpotlight
import Foundation
import MobileCoreServices
import Shared
import Web

private let browsingActivityType: String = "com.brave.ios.browsing"

private let searchableIndex = CSSearchableIndex(name: "firefox")

extension TabDataValues {
  private struct UserActivityTabHelperKey: TabDataKey {
    static var defaultValue: UserActivityTabHelper?
  }

  var userActivityHelper: UserActivityTabHelper? {
    get { self[UserActivityTabHelperKey.self] }
    set { self[UserActivityTabHelperKey.self] = newValue }
  }
}

class UserActivityTabHelper: TabObserver {
  private weak var tab: (any TabState)?
  private var userActivity: NSUserActivity?

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  func tabWasShown(_ tab: some TabState) {
    userActivity?.becomeCurrent()
  }

  func tabWasHidden(_ tab: some TabState) {
    userActivity?.resignCurrent()
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    userActivity?.invalidate()
    tab.removeObserver(self)
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    if let url = tab.visibleURL {
      setUserActivityForTab(tab, url: url)
    }
  }

  class func clearSearchIndex(completionHandler: ((Error?) -> Void)? = nil) {
    searchableIndex.deleteAllSearchableItems(completionHandler: completionHandler)
  }

  fileprivate func setUserActivityForTab(_ tab: some TabState, url: URL) {
    guard !tab.isPrivate, url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url)
    else {
      userActivity?.resignCurrent()
      userActivity = nil
      return
    }

    userActivity?.invalidate()

    let userActivity = NSUserActivity(activityType: browsingActivityType)
    userActivity.webpageURL = url
    userActivity.becomeCurrent()

    self.userActivity = userActivity
  }
}
