// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Web
import WebKit

enum ReadabilityOperationResult {
  case success(ReadabilityResult)
  case error(NSError)
  case timeout
}

class ReadabilityOperation: Operation {
  private var url: URL
  private var semaphore: DispatchSemaphore
  private var tab: (any TabState)!
  private var readerModeCache: ReaderModeCache
  var result: ReadabilityOperationResult?

  init(url: URL, readerModeCache: ReaderModeCache) {
    self.url = url
    self.semaphore = DispatchSemaphore(value: 0)
    self.readerModeCache = readerModeCache
  }

  override func main() {
    if self.isCancelled {
      return
    }

    // Setup a tab, attach a Readability helper. Kick all this off on the main thread since UIKit
    // and WebKit are not safe from other threads.

    DispatchQueue.main.async {
      let tab = TabStateFactory.create(with: .init(braveCore: nil))
      tab.browserData = .init(tab: tab, tabGeneratorAPI: nil)
      tab.createWebView()
      tab.addObserver(self)
      self.tab = tab

      let readerMode = ReaderModeScriptHandler()
      readerMode.delegate = self
      self.tab.browserData?.addContentScript(
        readerMode,
        name: ReaderModeScriptHandler.scriptName,
        contentWorld: ReaderModeScriptHandler.scriptSandbox
      )

      // Load the page in the webview. This either fails with a navigation error, or we
      // get a readability callback. Or it takes too long, in which case the semaphore
      // times out. The script on the page will retry every 500ms for 10 seconds.
      self.tab.loadRequest(URLRequest(url: self.url))
    }
    let timeout = DispatchTime.now() + .seconds(10)
    if semaphore.wait(timeout: timeout) == .timedOut {
      result = ReadabilityOperationResult.timeout
    }

    DispatchQueue.main.async {
      self.tab = nil
    }

    // Maybe this is where we should store stuff in the cache / run a callback?

    if let result = self.result {
      switch result {
      case .timeout:
        // Don't do anything on timeout
        break
      case .success(let readabilityResult):
        Task {
          try await readerModeCache.put(url, readabilityResult)
        }
      case .error(_):
        // TODO Not entitely sure what to do on error. Needs UX discussion and followup bug.
        break
      }
    }
  }
}

extension ReadabilityOperation: TabObserver {
  func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    result = ReadabilityOperationResult.error(error as NSError)
    semaphore.signal()
  }

  func tabDidFinishNavigation(_ tab: some TabState) {
    tab.evaluateJavaScript(
      functionName: "\(readerModeNamespace).checkReadability",
      contentWorld: ReaderModeScriptHandler.scriptSandbox
    )
  }
}

extension ReadabilityOperation: ReaderModeScriptHandlerDelegate {
  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didChangeReaderModeState state: ReaderModeState,
    forTab tab: some TabState
  ) {
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didDisplayReaderizedContentForTab tab: some TabState
  ) {
  }

  func readerMode(
    _ readerMode: ReaderModeScriptHandler,
    didParseReadabilityResult readabilityResult: ReadabilityResult,
    forTab tab: some TabState
  ) {
    guard tab === self.tab else {
      return
    }

    result = ReadabilityOperationResult.success(readabilityResult)
    semaphore.signal()
  }
}

class ReadabilityService {
  static let sharedInstance = ReadabilityService()
  private let readabilityServiceDefaultConcurrency = 1

  private var queue: OperationQueue

  init() {
    queue = OperationQueue()
    queue.maxConcurrentOperationCount = readabilityServiceDefaultConcurrency
  }

  func process(_ url: URL, cache: ReaderModeCache) {
    queue.addOperation(ReadabilityOperation(url: url, readerModeCache: cache))
  }
}
