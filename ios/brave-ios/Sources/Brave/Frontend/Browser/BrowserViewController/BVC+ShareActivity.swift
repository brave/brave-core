// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import Web

extension BrowserViewController: ShareActivityProvider {
  var shareSelectedTab: (any TabState)? { tabManager.selectedTab }
  var shareProfileController: BraveProfileController { profileController }
  var shareIsPrivateBrowsing: Bool { privateBrowsingManager.isPrivateBrowsing }
  var shareFeedDataSource: FeedDataSource? { feedDataSource }
  var shareCanAddSearchEngine: Bool {
    guard let tab = tabManager.selectedTab else { return false }
    return evaluateWebsiteSupportOpenSearchEngine(in: tab)
  }

  func shareShowSubmitReport(for url: URL) { showSubmitReportView(for: url) }
  func shareToggleReaderMode() { toggleReaderMode() }
  func shareDisplayPageZoom() { displayPageZoomDialog() }
  func shareAddSearchEngine() { addCustomSearchEngineForFocusedElement() }
  func shareDisplayCertificate() { displayPageCertificateInfo() }
  func shareCleanUp() { showQueuedAlertIfAvailable() }
}
