// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension TabDataValues {
  private struct AIChatWebUITabHelperKey: TabDataKey {
    static var defaultValue: AIChatWebUITabHelper?
  }

  public var aiChatWebUITabHelper: AIChatWebUITabHelper? {
    get { self[AIChatWebUITabHelperKey.self] }
    set { self[AIChatWebUITabHelperKey.self] = newValue }
  }
}

public class AIChatWebUITabHelper: NSObject, TabObserver {
  private weak var tab: ChromiumTabState?
  private weak var controller: AIChatCommunicationController?
  weak var delegate: AIChatCommunicationProtocol?

  public init?(tab: some TabState, browser: AIChatCommunicationProtocol) {
    guard let chromiumTabState = tab as? ChromiumTabState else {
      return nil
    }

    self.tab = chromiumTabState
    self.delegate = browser
    self.controller = self.tab?.webView?.aiChatController()

    controller?.delegate = browser
  }
}
