// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

class MockMessageHandler: NSObject, WKScriptMessageHandlerWithReply, AsyncSequence, AsyncIteratorProtocol {
  enum LoadError: Error {
    case timedOut
  }
  
  typealias AsyncIterator = MockMessageHandler
  typealias Element = WKScriptMessage
  typealias Callback = (Element) -> Any?
  
  private let callback: Callback
  private var messages: [WKScriptMessage] = []
  private var timeout: TimeInterval
  private var start: Date
  
  init(timeout: TimeInterval = 60, callback: @escaping Callback) {
    self.callback = callback
    self.start = Date()
    self.timeout = timeout
  }
  
  // MARK: - WKScriptMessageHandlerWithReply
  func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    let reply = callback(message)
    replyHandler(reply, nil)
    messages.append(message)
  }
  
  func next() async throws -> Element? {
    while Date().timeIntervalSince(start) < timeout {
      guard !messages.isEmpty else {
        try await Task.sleep(seconds: 0.5)
        continue
      }
      
      return messages.removeFirst()
    }
    
    throw LoadError.timedOut
  }
  
  func makeAsyncIterator() -> MockMessageHandler {
    self.start = Date()
    return self
  }
}
