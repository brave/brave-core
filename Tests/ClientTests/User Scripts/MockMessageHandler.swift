// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

class MockMessageHandler: NSObject, WKScriptMessageHandlerWithReply {
  typealias Callback = (WKScriptMessage) -> Any?
  private let callback: Callback
  
  init(callback: @escaping Callback) {
    self.callback = callback
  }
  
  // MARK: - WKScriptMessageHandlerWithReply
  func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    let reply = callback(message)
    replyHandler(reply, nil)
  }
}
