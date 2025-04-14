// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit
import Web
import WebKit
import os.log

/// Handles screenshots for a given tab, including pages with non-webview content.
class ScreenshotHelper {
  var viewIsVisible = false

  fileprivate weak var tabManager: TabManager?

  init(tabManager: TabManager) {
    self.tabManager = tabManager
  }

  func takeScreenshot(_ tab: some TabState) {
    guard let url = tab.visibleURL else {
      Logger.module.error("Tab webView or url is nil")
      tab.browserData?.setScreenshot(nil)
      return
    }

    if InternalURL(url)?.isAboutHomeURL == true {
      if let homePanel = tabManager?.selectedTab?.newTabPageViewController {
        let screenshot = homePanel.view.screenshot(quality: UIConstants.activeScreenshotQuality)
        tab.browserData?.setScreenshot(screenshot)
      } else {
        tab.browserData?.setScreenshot(nil)
      }
    } else {
      if !tab.canTakeSnapshot {
        return
      }
      tab.takeSnapshot(rect: .null) { [weak tab] image in
        guard let tab else { return }
        if let image = image {
          tab.browserData?.setScreenshot(image)
        } else {
          Logger.module.error("Cannot snapshot Tab Screenshot - No error description")
          tab.browserData?.setScreenshot(nil)
        }
      }
    }
  }

  /// Takes a screenshot after a small delay.
  /// Trying to take a screenshot immediately after didFinishNavigation results in a screenshot
  /// of the previous page, presumably due to an iOS bug. Adding a brief delay fixes this.
  func takeDelayedScreenshot(_ tab: some TabState) {
    let time = DispatchTime.now() + Double(Int64(100 * NSEC_PER_MSEC)) / Double(NSEC_PER_SEC)
    DispatchQueue.main.asyncAfter(deadline: time) {
      // If the view controller isn't visible, the screenshot will be blank.
      // Wait until the view controller is visible again to take the screenshot.
      guard self.viewIsVisible else {
        tab.pendingScreenshot = true
        return
      }

      self.takeScreenshot(tab)
    }
  }

  func takePendingScreenshots(_ tabs: [any TabState]) {
    for tab in tabs where tab.pendingScreenshot == true {
      tab.pendingScreenshot = false
      takeDelayedScreenshot(tab)
    }
  }
}
