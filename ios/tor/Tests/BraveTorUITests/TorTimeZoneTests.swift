// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
#if os(iOS)
import BraveTor
import TorWebKit
import WebKit
import XCTest
@testable import BraveTorUI

@MainActor
final class TorTimeZoneTests: XCTestCase {
  func testNativeUTCOverridesDateIntlFramesAndWorkersWithoutChangingOtherPools() async throws {
    let rules = try await TorWebPolicy.compileRules()
    let configuration = try TorWebPolicy.configuration(
      endpoint: TorEndpoint(listener: "127.0.0.1:9050"), rules: rules,
      isolationID: "utc-test", javaScriptEnabled: true
    )
    let controlConfiguration = WKWebViewConfiguration()
    controlConfiguration.websiteDataStore = .nonPersistent()
    controlConfiguration.processPool = try XCTUnwrap(BraveTorProcessPoolForTimeZone("America/Chicago"))
    let control = WKWebView(frame: .zero, configuration: controlConfiguration)
    let browser = WKWebView(frame: .zero, configuration: configuration)
    let ready = expectation(description: "Local documents ready")
    ready.expectedFulfillmentCount = 2
    let loader = LocalDocumentObserver { ready.fulfill() }
    control.navigationDelegate = loader
    browser.navigationDelegate = loader
    control.loadHTMLString("<!doctype html><html><body></body></html>", baseURL: nil)
    browser.loadHTMLString("<!doctype html><html><body></body></html>", baseURL: nil)
    await fulfillment(of: [ready], timeout: 10)
    let chicago = try await control.callAsyncJavaScript(
      "return Intl.DateTimeFormat().resolvedOptions().timeZone;", arguments: [:], in: nil, contentWorld: .page
    )
    XCTAssertEqual(chicago as? String, "America/Chicago")
    let result = try await browser.callAsyncJavaScript("""
      const date = new Date(2026, 0, 2, 3, 4, 5);
      const workerResult = await Promise.race([new Promise((resolve, reject) => {
        const blob = new Blob(['postMessage([Intl.DateTimeFormat().resolvedOptions().timeZone, new Date().getTimezoneOffset()])'], {type:'application/javascript'});
        const worker = new Worker(URL.createObjectURL(blob));
        worker.onmessage = event => { worker.terminate(); resolve(event.data); };
        worker.onerror = reject;
      }), new Promise((_, reject) => setTimeout(() => reject(new Error('worker timeout')), 10000))]);
      const iframeResult = await new Promise(resolve => {
        const frame = document.createElement('iframe');
        frame.srcdoc = '<script>parent.postMessage([Intl.DateTimeFormat().resolvedOptions().timeZone, new Date().getTimezoneOffset()], "*")<\\/script>';
        window.addEventListener('message', event => {
          if (event.source === frame.contentWindow) { frame.remove(); resolve(event.data); }
        }, {once:true});
        document.documentElement.appendChild(frame);
      });
      return {
        zone: Intl.DateTimeFormat().resolvedOptions().timeZone,
        offset: date.getTimezoneOffset(), iso: date.toISOString(),
        winter: new Date('2026-01-01T00:00:00Z').getTimezoneOffset(),
        summer: new Date('2026-07-01T00:00:00Z').getTimezoneOffset(),
        string: date.toString(), workerZone: workerResult[0], workerOffset: workerResult[1],
        frameZone: iframeResult[0], frameOffset: iframeResult[1]
      };
      """, arguments: [:], in: nil, contentWorld: .page) as? [String: Any]
    let values = try XCTUnwrap(result)
    XCTAssertEqual(values["zone"] as? String, "UTC")
    XCTAssertEqual(values["iso"] as? String, "2026-01-02T03:04:05.000Z")
    XCTAssertTrue((values["string"] as? String)?.contains("GMT+0000") == true)
    for key in ["offset", "winter", "summer", "workerOffset", "frameOffset"] {
      XCTAssertEqual(values[key] as? Int, 0, key)
    }
    XCTAssertEqual(values["workerZone"] as? String, "UTC")
    XCTAssertEqual(values["frameZone"] as? String, "UTC")
    let unchanged = try await control.callAsyncJavaScript(
      "return Intl.DateTimeFormat().resolvedOptions().timeZone;", arguments: [:], in: nil, contentWorld: .page
    )
    XCTAssertEqual(unchanged as? String, "America/Chicago")
    withExtendedLifetime(loader) {}
  }

  private final class LocalDocumentObserver: NSObject, WKNavigationDelegate {
    let onFinish: () -> Void
    init(onFinish: @escaping () -> Void) { self.onFinish = onFinish }
    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) { onFinish() }
  }
}
#endif
