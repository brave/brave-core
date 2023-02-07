/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import Shared
import BraveShared

private let apostropheEncoded = "%27"

extension WKWebView {
  // Use JS to redirect the page without adding a history entry
  func replaceLocation(with url: URL) {
    let safeUrl = url.absoluteString.replacingOccurrences(of: "'", with: apostropheEncoded)
    evaluateSafeJavaScript(functionName: "location.replace", args: ["'\(safeUrl)'"], contentWorld: .defaultClient, escapeArgs: false, asFunction: true, completion: nil)
  }
}

extension InternalSchemeResponse {
  func generateInvalidSchemeResponse(url: String, for originURL: URL) -> (URLResponse, Data)? {
    // Same validation as in WKNavigationDelegate -> decidePolicyFor
    guard let scheme = URL(string: url)?.scheme,
      ["http", "https", "file", "about", InternalURL.scheme].contains(scheme)
    else {

      let html = """
        <!DOCTYPE html>
        <html>
            <head>
                <meta name="referrer" content="no-referrer">
            </head>
            <body>
                <h1>\(Strings.genericErrorBody)</h1>
            </body>
        </html>
        """
      let data = html.data(using: .utf8)!
      let response = InternalSchemeHandler.response(forUrl: originURL)
      return (response, data)
    }
    return nil
  }

  func generateResponseThatRedirects(toUrl url: URL) -> (URLResponse, Data) {
    var urlString: String
    if InternalURL.isValid(url: url), let authUrl = InternalURL.authorize(url: url) {
      urlString = authUrl.absoluteString
    } else {
      urlString = url.absoluteString
    }

    if let invalidSchemeResponse = generateInvalidSchemeResponse(url: urlString, for: url) {
      return invalidSchemeResponse
    }

    urlString = urlString.replacingOccurrences(of: "'", with: apostropheEncoded)

    let html = """
      <!DOCTYPE html>
      <html>
          <head>
              <meta name="referrer" content="no-referrer">
              <script>
                  location.replace('\(urlString)');
              </script>
          </head>
      </html>
      """

    let data = html.data(using: .utf8)!
    let response = InternalSchemeHandler.response(forUrl: url)
    return (response, data)
  }
}

/// Handles requests to /about/sessionrestore to restore session history.
public class SessionRestoreHandler: InternalSchemeResponse {
  public static let path = "sessionrestore"

  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let _url = request.url, let url = InternalURL(_url) else { return nil }

    // Handle the 'url='query param
    if let urlParam = url.extractedUrlParam {
      return generateResponseThatRedirects(toUrl: urlParam)
    }

    // From here on, handle 'history=' query param
    let response = InternalSchemeHandler.response(forUrl: url.url)
    guard let sessionRestorePath = Bundle.module.path(forResource: "SessionRestore", ofType: "html"),
      var html = try? String(contentsOfFile: sessionRestorePath)
    else {
      assert(false)
      return nil
    }

    html = html.replacingOccurrences(of: "%INSERT_UUID_VALUE%", with: InternalURL.uuid)
    html = html.replacingOccurrences(of: "%security_token%", with: SessionRestoreScriptHandler.scriptId)

    guard let data = html.data(using: .utf8) else {
      assert(false)
      return nil
    }

    return (response, data)
  }
  
  public init() { }
}
