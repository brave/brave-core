// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

class DownloadQueueTests: XCTestCase {

  func test_enqueue_whenItHasDownloads_sendsCorrectMessage() {
    let (sut, delegate) = makeSUT()
    let download = DownloadSpy()    
    sut.downloads = [Download(), Download()]

    sut.enqueue(download)

    XCTAssertEqual(delegate.receivedMessages, [.didStartDownload])
    XCTAssertEqual(download.receivedMessages, [.resume])
    XCTAssertNotNil(download.delegate as? DownloadQueue)
    XCTAssertEqual(sut.downloads.count, 3)
  }

  func test_cancelAll_sendsCancelMessageForIncompleteDownloads() {
    let (sut, _) = makeSUT()
    let download1 = DownloadSpy()
    let download2 = DownloadSpy()
    let download3 = DownloadSpy()
    sut.downloads = [download1, download2, download3]

    sut.cancelAll()

    XCTAssertFalse(download1.isComplete)
    XCTAssertFalse(download2.isComplete)
    XCTAssertFalse(download3.isComplete)
    XCTAssertEqual(download1.receivedMessages, [.cancel])
    XCTAssertEqual(download2.receivedMessages, [.cancel])
    XCTAssertEqual(download3.receivedMessages, [.cancel])
  }

  func test_pauseAll_sendsPauseMessageForIncompleteDownloads() {
    let (sut, _) = makeSUT()
    let download1 = DownloadSpy()
    let download2 = DownloadSpy()
    let download3 = DownloadSpy()
    sut.downloads = [download1, download2, download3]

    sut.pauseAll()

    XCTAssertFalse(download1.isComplete)
    XCTAssertFalse(download2.isComplete)
    XCTAssertFalse(download3.isComplete)
    XCTAssertEqual(download1.receivedMessages, [.pause])
    XCTAssertEqual(download2.receivedMessages, [.pause])
    XCTAssertEqual(download3.receivedMessages, [.pause])
  }

  func test_resumeAll_sendsResumeMessageForIncompleteDownloads() {
    let (sut, _) = makeSUT()
    let download1 = DownloadSpy()
    let download2 = DownloadSpy()
    let download3 = DownloadSpy()
    sut.downloads = [download1, download2, download3]

    sut.resumeAll()

    XCTAssertFalse(download1.isComplete)
    XCTAssertFalse(download2.isComplete)
    XCTAssertFalse(download3.isComplete)
    XCTAssertEqual(download1.receivedMessages, [.resume])
    XCTAssertEqual(download2.receivedMessages, [.resume])
    XCTAssertEqual(download3.receivedMessages, [.resume])
  }

  func test_downloadDidCompleteWithError_whenErrorIsEmpty_doNothing() {
    let (sut, delegate) = makeSUT()
    let download = Download()
    sut.downloads = [download]

    sut.download(download, didCompleteWithError: nil)

    XCTAssertEqual(delegate.receivedMessages, [])
  }

  func test_downloadDidCompleteWithError_whenDownloadsAreEmpty_doNothing() {
    let (sut, delegate) = makeSUT()
    let download = Download()
    sut.downloads = []

    let error = NSError(domain: "download.error", code: 0)
    sut.download(download, didCompleteWithError: error)

    XCTAssertEqual(delegate.receivedMessages, [])
  }

  func test_downloadDidCompleteWithError_whenItHasMoreThanOneDownload_doNothing() {
    let (sut, delegate) = makeSUT()
    let download1 = Download()
    let download2 = Download()
    sut.downloads = [download1, download2]

    let error = NSError(domain: "download.error", code: 0)
    sut.download(download1, didCompleteWithError: error)

    XCTAssertEqual(delegate.receivedMessages, [])
  }

  func test_downloadDidCompleteWithError_sendsCorrectMessage() {
    let (sut, delegate) = makeSUT()
    let download = Download()
    sut.downloads = [download]

    let error = NSError(domain: "download.error", code: 0)
    sut.download(download, didCompleteWithError: error)

    XCTAssertEqual(delegate.receivedMessages, [.didCompleteWithError(error: .downloadError)])
  }

  func test_downloadDidDownloadBytes_sendsCorrectMessage() {
    let (sut, delegate) = makeSUT()
    let download = Download()

    sut.download(download, didDownloadBytes: 1000)
    sut.download(download, didDownloadBytes: 1000)
    sut.download(download, didDownloadBytes: 1000)

    XCTAssertEqual(delegate.receivedMessages, [.didDownloadCombinedBytes(bytes: 1000),
                                               .didDownloadCombinedBytes(bytes: 2000),
                                               .didDownloadCombinedBytes(bytes: 3000)])
  }

  func test_downloadDidFinishDownloadingTo_whenDownloadsAreEmpty_doNothing() {
    let (sut, delegate) = makeSUT()
    let download = Download()
    sut.downloads = []

    let location = URL(string: "https://some-location")!
    sut.download(download, didFinishDownloadingTo: location)

    XCTAssertEqual(delegate.receivedMessages, [])
  }

  func test_downloadDidFinishDownloadingTo_whenItHasDownload_removesCorrectDownload() {
    let (sut, _) = makeSUT()
    let download1 = Download()
    let download2 = Download()
    sut.downloads = [download1, download2]

    let location = URL(string: "https://some-location")!
    sut.download(download1, didFinishDownloadingTo: location)

    XCTAssertEqual(sut.downloads, [download2])
  }

  func test_downloadDidFinishDownloadingTo_whenItHasDownloads_sendsCorrectMessage() {
    let (sut, delegate) = makeSUT()
    let download1 = Download()
    let download2 = Download()
    sut.downloads = [download1, download2]

    let location = URL(string: "https://some-location")!
    sut.download(download1, didFinishDownloadingTo: location)

    XCTAssertEqual(delegate.receivedMessages, [.didFinishDownloadingTo(location: location)])
  }

  func test_downloadDidFinishDownloadingTo_whenLastDonwload_sendsCorrectMessage() {
    let (sut, delegate) = makeSUT()
    let download = Download()
    sut.downloads = [download]

    let location = URL(string: "https://some-location")!
    sut.download(download, didFinishDownloadingTo: location)

    XCTAssertEqual(delegate.receivedMessages, [.didFinishDownloadingTo(location: location),
                                               .didCompleteWithError(error: nil)])
  }
}

// MARK: - Tests Helpers

private func makeSUT() -> (sut: DownloadQueue, delegate: DownloadQueueDelegateSpy) {
  let delegate = DownloadQueueDelegateSpy()
  let sut = DownloadQueue()
  sut.delegate = delegate

  return (sut, delegate)
}

private class DownloadSpy: Download {
    enum Message {
      case resume
      case cancel
      case pause
    }

    var receivedMessages: [Message] = []

    override func resume() {
      receivedMessages.append(.resume)
    }
    
    override func cancel() {
      receivedMessages.append(.cancel)
    }

    override func pause() {
      receivedMessages.append(.pause)
    }
}

private class DownloadQueueDelegateSpy: DownloadQueueDelegate {
  enum DownloadQueueError: Error {
      case downloadError
  }

  enum Message: Equatable {
    case didStartDownload
    case didCompleteWithError(error: DownloadQueueError?)
    case didDownloadCombinedBytes(bytes: Int64)
    case didFinishDownloadingTo(location: URL)
  }

  var receivedMessages: [Message] = []

  func downloadQueue(_ downloadQueue: DownloadQueue, didStartDownload download: Download) {
    receivedMessages.append(.didStartDownload)
  }

  func downloadQueue(_ downloadQueue: DownloadQueue, didDownloadCombinedBytes combinedBytesDownloaded: Int64, combinedTotalBytesExpected: Int64?) {
    receivedMessages.append(.didDownloadCombinedBytes(bytes: combinedBytesDownloaded))
  }

  func downloadQueue(_ downloadQueue: DownloadQueue, download: Download, didFinishDownloadingTo location: URL) {
    receivedMessages.append(.didFinishDownloadingTo(location: location))
  }

  func downloadQueue(_ downloadQueue: DownloadQueue, didCompleteWithError error: Error?) {
    receivedMessages.append(.didCompleteWithError(error: error != nil ? .downloadError : nil))
  }
}
