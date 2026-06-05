// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import Web

extension BrowserViewController {
  func presentActivityViewController(
    _ url: URL,
    tab: (any TabState)? = nil,
    source: SharePopoverSource
  ) {
    presentShareActivity(
      url: url,
      tab: tab,
      syncAPI: profileController.syncAPI,
      sendTabAPI: profileController.sendTabAPI,
      feedDataSource: feedDataSource,
      isBraveNewsAvailable: profileController.profile.prefs.isBraveNewsAvailable,
      source: source,
      callbacks: .init(
        onToggleReaderMode: { [weak self] in self?.toggleReaderMode() },
        onDisplayPageZoom: { [weak self] in self?.displayPageZoomDialog() },
        onAddSearchEngine: { [weak self] in
          guard let self, let tab else { return }
          self.evaluateWebsiteSupportOpenSearchEngine(in: tab)
          self.addCustomSearchEngineForFocusedElement()
        },
        onDisplayCertificate: { [weak self] in self?.displayPageCertificateInfo() },
        onShowSubmitReport: { [weak self] url in self?.showSubmitReportView(for: url) },
        onCleanUp: { [weak self] in self?.showQueuedAlertIfAvailable() }
      )
    )
  }
}
