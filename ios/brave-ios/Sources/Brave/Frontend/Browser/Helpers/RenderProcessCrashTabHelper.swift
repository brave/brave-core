// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Web

extension TabDataValues {
  private struct RenderProcessCrashTabHelperKey: TabDataKey {
    static var defaultValue: RenderProcessCrashTabHelper?
  }
  var renderProcessCrash: RenderProcessCrashTabHelper? {
    get { self[RenderProcessCrashTabHelperKey.self] }
    set { self[RenderProcessCrashTabHelperKey.self] = newValue }
  }
}

/// Reloads a tab when its render process terminates, but only once per URL until that URL
/// successfully loads again, so a page that crashes on load doesn't end up in a reload loop.
class RenderProcessCrashTabHelper: TabObserver {
  private weak var tab: (any TabState)?

  /// The URL we last attempted to recover by reloading
  private var recoveryAttemptURL: URL?

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  func tabRenderProcessDidTerminate(_ tab: some TabState) {
    guard let url = tab.lastCommittedURL else { return }
    guard recoveryAttemptURL != url else { return }
    recoveryAttemptURL = url
    tab.reload()
  }

  func tabDidFinishNavigation(_ tab: some TabState) {
    recoveryAttemptURL = nil
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
