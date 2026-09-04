// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Preferences
import Shared
import WebKit

public class ReaderModeHandler: InternalSchemeResponse {
  public static let path = InternalURL.Path.readermode.rawValue
  internal static var readerModeCache: ReaderModeCache = DiskReaderModeCache.sharedInstance
  private static let readerModeStyleHash = "sha256-L2W8+0446ay9/L1oMrgucknQXag570zwgQrHwE68qbQ="

  public init() {}

  public func response(forRequest request: URLRequest) async -> (URLResponse, Data)? {
    guard let _url = request.url,
      let url = InternalURL(_url),
      let readerModeUrl = url.extractedUrlParam
    else {
      return nil
    }

    // Decode the original page's response headers
    var headers = [String: String]()
    if let base64EncodedHeaders = _url.getQuery()["headers"]?.unescape(),
      let data = Data(base64Encoded: base64EncodedHeaders),
      let decodedHeaders = try? JSONSerialization.jsonObject(with: data) as? [String: String]
    {
      headers = decodedHeaders
    }

    headers = headers.filter({
      let key = $0.key.lowercased()

      // These are the only headers kept from the original page
      return key == "access-control-allow-origin" || key == "content-security-policy"
        || key == "strict-transport-security" || key == "content-language"
    })

    // Tighten security by adding some of our own headers
    headers["X-Frame-Options"] = "DENY"
    headers["X-Content-Type-Options"] = "nosniff"
    headers["Referrer-Policy"] = "no-referrer"
    headers["Cache-Control"] = "private, s-maxage=0, max-age=0, must-revalidate"

    // Add Generic headers
    headers["Content-Type"] = "text/html; charset=UTF-8"

    // Handle CSP header
    // Must generate a unique nonce, every single time as per Content-Policy spec.
    let setTitleNonce = UUID().uuidString.replacingOccurrences(of: "-", with: "")

    // Only the img-src directive is adopted from the original page's CSP. Remove the original
    // header (its key casing may differ) so it is not emitted alongside our own policy.
    let originalCSP = headers.first(where: {
      $0.key.lowercased() == "content-security-policy"
    })?.value
    headers = headers.filter({ $0.key.lowercased() != "content-security-policy" })
    headers["Content-Security-Policy"] = ReaderModeHandler.contentSecurityPolicy(
      originalCSP: originalCSP,
      scriptNonce: setTitleNonce
    )

    if url.url.lastPathComponent == "page-exists" {
      let statusCode = await ReaderModeHandler.readerModeCache.contains(readerModeUrl) ? 200 : 400
      if let response = HTTPURLResponse(
        url: url.url,
        statusCode: statusCode,
        httpVersion: "HTTP/1.1",
        headerFields: headers
      ) {
        return (response, Data())
      }
      return nil
    }

    // From here on, handle 'url=' query param
    if !readerModeUrl.isSecureWebPage() {
      if let response = HTTPURLResponse(
        url: url.url,
        statusCode: 500,
        httpVersion: "HTTP/1.1",
        headerFields: ["Content-Type": "text/html; charset=UTF-8"]
      ) {
        return (response, Data())
      }
      return nil
    }

    do {
      let readabilityResult = try await ReaderModeHandler.readerModeCache.get(readerModeUrl)
      // We have this page in our cache, so we can display it. Just grab the correct style from the
      // profile and then generate HTML from the Readability results.
      var readerModeStyle = await MainActor.run {
        return defaultReaderModeStyle
      }

      if let encodedString = Preferences.ReaderMode.style.value {
        if let style = ReaderModeStyle(encodedString: encodedString) {
          readerModeStyle = style
        }
      }

      if let html = await ReaderModeUtils.generateReaderContent(
        readabilityResult,
        initialStyle: readerModeStyle,
        titleNonce: setTitleNonce
      ) {
        // Apply a Content Security Policy that disallows everything except images from anywhere and fonts and css from our internal server
        guard
          let response = HTTPURLResponse(
            url: url.url,
            statusCode: 200,
            httpVersion: "HTTP/1.1",
            headerFields: headers
          )
        else {
          return nil
        }
        let data = Data(html.utf8)
        return (response, data)
      }
    } catch {
      // Attempted to load this page without actually having previously calling `readerize` on the
      // underlying page and storing the result in `ReaderModeHandler.readerModeCache`
      return nil
    }

    assert(false)
    return nil
  }

  /// Builds the Content-Security-Policy header value for a reader mode page.
  ///
  /// Only the `img-src` directive is adopted from the original page's CSP so that images the
  /// original page allowed can still load. Every other directive is ignored so that a page cannot
  /// weaken the reader mode policy, e.g. via `script-src-elem`/`script-src-attr` which take
  /// precedence over the nonce-based `script-src` for inline event handlers, or `frame-src` which
  /// would allow framing internal pages.
  static func contentSecurityPolicy(originalCSP: String?, scriptNonce: String) -> String {
    // Create our own CSPs
    var policies = [
      ("default-src", "'none'"),
      ("base-uri", "'none'"),
      ("form-action", "'none'"),
      ("frame-ancestors", "'none'"),
      //("sandbox", ""),                  // Do not enable `sandbox` as it causes `Wikipedia` to not work and possibly other pages
      ("upgrade-insecure-requests", "1"),
      ("img-src", "*"),
      ("style-src", "\(InternalURL.baseUrl) '\(ReaderModeHandler.readerModeStyleHash)'"),
      ("font-src", "\(InternalURL.baseUrl)"),
      ("script-src", "'nonce-\(scriptNonce)'"),
    ]

    if let originalCSP {
      let policiesStrings = originalCSP.components(separatedBy: ";")
        .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
      for policy in policiesStrings {
        let components = policy.components(separatedBy: " ").filter({ !$0.isEmpty })
        guard components.count > 1, components[0].lowercased() == "img-src" else { continue }
        // Strip control characters so the value cannot break header serialization
        let value = components[1...].joined(separator: " ").filter({ !$0.isNewline })
        guard !value.isEmpty else { continue }
        policies.removeAll(where: { key, _ in key == "img-src" })
        policies.append(("img-src", value))
      }
    }

    return policies.map({ (key, value) in
      return value.isEmpty ? "\(key);" : "\(key) \(value);"
    }).joined(separator: " ")
  }
}
