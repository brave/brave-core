// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

class MockMessageHandler: NSObject, WKScriptMessageHandlerWithReply {
  typealias Callback = (WKScriptMessage) -> Any?
  private let callback: Callback
  private var streamCallback: ((WKScriptMessage) -> Void)?
  private var streamTimer: Timer?
  private var messages: [WKScriptMessage] = []
  
  init(callback: @escaping Callback) {
    self.callback = callback
  }
  
  // MARK: - WKScriptMessageHandlerWithReply
  func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    let reply = callback(message)
    replyHandler(reply, nil)
    
    if let callback = streamCallback {
      callback(message)
    } else {
      messages.append(message)
    }
  }
  
  func messagesStream(timeout: TimeInterval = 10) -> AsyncStream<WKScriptMessage> {
    return AsyncStream { continuation in
      if messages.isEmpty {
        // Add a timeout in case the script is bad and the handler never triggers
        // (so we don't the test hang for ever)
        streamTimer = Timer.scheduledTimer(withTimeInterval: timeout, repeats: false, block: { _ in
          self.streamCallback = nil
          continuation.finish()
        })
      }
      
      while !messages.isEmpty {
        continuation.yield(messages.removeFirst())
      }
      
      streamCallback = { message in
        self.streamTimer?.invalidate()
        continuation.yield(message)
      }
      
      continuation.onTermination = { @Sendable _ in
        self.streamTimer?.invalidate()
        self.streamCallback = nil
      }
    }
  }
}
