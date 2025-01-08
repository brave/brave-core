// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import XCTest

@testable import Brave

class DownloadHelperTests: XCTestCase {

  func test_init_whenMIMETypeIsNil_initializeCorrectly() {
    let response = anyResponse(mimeType: nil)

    var sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: true
    )
    XCTAssertNotNil(sut)

    sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: false
    )
    XCTAssertNotNil(sut)

    sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: false
    )
    XCTAssertNotNil(sut)

    sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: true
    )
    XCTAssertNotNil(sut)
  }

  func test_init_whenMIMETypeIsNotOctetStream_initializeCorrectly() {
    for mimeType in allMIMETypes() {
      if mimeType == MIMEType.octetStream { continue }

      let response = anyResponse(mimeType: mimeType)

      var sut = DownloadHelper(
        request: anyRequest(),
        response: response,
        cookieStore: cookieStore(),
        canShowInWebView: true
      )
      XCTAssertNil(sut)

      sut = DownloadHelper(
        request: anyRequest(),
        response: response,
        cookieStore: cookieStore(),
        canShowInWebView: false
      )
      XCTAssertNotNil(sut)

      sut = DownloadHelper(
        request: anyRequest(),
        response: response,
        cookieStore: cookieStore(),
        canShowInWebView: false
      )
      XCTAssertNotNil(sut)
    }
  }

  func test_init_whenMIMETypeIsOctetStream_initializeCorrectly() {
    let response = anyResponse(mimeType: MIMEType.octetStream)

    var sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: true
    )
    XCTAssertNotNil(sut)

    sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: false
    )
    XCTAssertNotNil(sut)

    sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: true
    )
    XCTAssertNotNil(sut)

    sut = DownloadHelper(
      request: anyRequest(),
      response: response,
      cookieStore: cookieStore(),
      canShowInWebView: false
    )
    XCTAssertNotNil(sut)
  }

  func test_downloadAlert_whenRequestURLIsWrong_deliversEmptyResult() {
    let request = anyRequest(urlString: "wrong-url.com")
    let sut = DownloadHelper(
      request: request,
      response: anyResponse(mimeType: nil),
      cookieStore: cookieStore(),
      canShowInWebView: true
    )

    let downloadAlert = sut?.downloadAlert(from: UIView(), okAction: { _ in })

    XCTAssertNil(downloadAlert)
  }

  func test_downloadAlert_deliversCorrectTitle() {
    let request = anyRequest(urlString: "http://some-domain.com/some-image.jpg")
    let sut = DownloadHelper(
      request: request,
      response: anyResponse(mimeType: nil),
      cookieStore: cookieStore(),
      canShowInWebView: true
    )

    let downloadAlert = sut?.downloadAlert(from: UIView(), okAction: { _ in })

    XCTAssertEqual(downloadAlert!.title!, "some-image.jpg - some-domain.com")
  }

  func test_downloadAlert_okActionButtonSendsCorrectMessage() {
    let request = anyRequest()
    let sut = DownloadHelper(
      request: request,
      response: anyResponse(mimeType: nil),
      cookieStore: cookieStore(),
      canShowInWebView: true
    )
    let okActionIndex = 0

    var receivedMessages = [String]()
    let okAction: (HTTPDownload) -> Void = { download in
      receivedMessages.append(download.request.url!.absoluteString)
    }
    let downloadAlert = sut?.downloadAlert(from: UIView(), okAction: okAction)
    downloadAlert?.tapButton(atIndex: okActionIndex)

    XCTAssertEqual(receivedMessages, [request.url!.absoluteString])
  }

  func test_downloadAlert_deliversCorrectCancelButton() {
    let sut = DownloadHelper(
      request: anyRequest(),
      response: anyResponse(mimeType: nil),
      cookieStore: cookieStore(),
      canShowInWebView: true
    )

    let downloadAlert = sut?.downloadAlert(from: UIView(), okAction: { _ in })

    XCTAssertEqual(downloadAlert!.actions.count, 2)
    XCTAssertEqual(downloadAlert!.actions.last!.style, .cancel)
  }

  // MARK: - Helpers

  private func anyRequest(urlString: String = "http://any-url.com") -> URLRequest {
    return URLRequest(
      url: URL(string: urlString)!,
      cachePolicy: anyCachePolicy(),
      timeoutInterval: 60.0
    )
  }

  private func anyResponse(mimeType: String?) -> URLResponse {
    return URLResponse(
      url: URL(string: "http://any-url.com")!,
      mimeType: mimeType,
      expectedContentLength: 10,
      textEncodingName: nil
    )
  }

  private func cookieStore() -> WKHTTPCookieStore {
    return WKWebsiteDataStore.`default`().httpCookieStore
  }

  private func anyCachePolicy() -> URLRequest.CachePolicy {
    return .useProtocolCachePolicy
  }

  private func allMIMETypes() -> [String] {
    return [
      MIMEType.bitmap,
      MIMEType.css,
      MIMEType.gif,
      MIMEType.javaScript,
      MIMEType.jpeg,
      MIMEType.html,
      MIMEType.octetStream,
      MIMEType.passbook,
      MIMEType.pdf,
      MIMEType.plainText,
      MIMEType.png,
      MIMEType.webP,
      MIMEType.xHTML,
    ]
  }
}

extension UIAlertController {
  fileprivate typealias AlertHandler = @convention(block) (UIAlertAction) -> Void

  fileprivate func tapButton(atIndex index: Int) {
    guard let block = actions[index].value(forKey: "handler") else { return }
    let handler = unsafeBitCast(block as AnyObject, to: AlertHandler.self)
    handler(actions[index])
  }
}
