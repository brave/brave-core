// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Foundation
import Shared
import WebKit

public class ReaderModeHandler: InternalSchemeResponse {
  private let profile: Profile
  private let braveCore: BraveCoreMain
  public static let path = InternalURL.Path.readermode.rawValue
  internal static var readerModeCache: ReaderModeCache = DiskReaderModeCache.sharedInstance
  private static let readerModeStyleHash = "sha256-L2W8+0446ay9/L1oMrgucknQXag570zwgQrHwE68qbQ="

  public init(profile: Profile, braveCore: BraveCoreMain) {
    self.profile = profile
    self.braveCore = braveCore
  }

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
      ("script-src", "'nonce-\(setTitleNonce)'"),
    ]

    // Parse CSP Header
    if let originalCSP = headers.first(where: { $0.key.lowercased() == "content-security-policy" })?
      .value
    {
      var originalPolicies = [(String, String)]()
      for policy in originalCSP.components(separatedBy: ";") {
        let components = policy.components(separatedBy: " ")
        if components.count == 1 {
          originalPolicies.append((policy, ""))
        } else {
          let key = components[0]
          let value = components[1...].joined(separator: " ")
          originalPolicies.append((key, value))
        }
      }

      // Remove unwanted policies
      originalPolicies.removeAll(where: { key, _ in
        key == "report-uri" || key == "report-to"
      })

      if originalPolicies.contains(where: { key, _ in key == "img-src" }) {
        policies.removeAll(where: { key, _ in key == "img-src" })
      }

      // Add original CSPs onto our own
      policies.append(contentsOf: originalPolicies)
    }

    headers["Content-Security-Policy"] = String(
      policies.map({ (key, value) in
        return value.isEmpty ? "\(key);" : "\(key) \(value);"
      }).joined(by: " ")
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
      var readerModeStyle = defaultReaderModeStyle
      if let dict = profile.prefs.dictionaryForKey(readerModeProfileKeyStyle) {
        if let style = ReaderModeStyle(dict: dict) {
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
      // This page has not been converted to reader mode yet. This happens when you for example add an
      // item via the app extension and the application has not yet had a change to readerize that
      // page in the background.
      //
      // What we do is simply queue the page in the ReadabilityService and then show our loading
      // screen, which will periodically call page-exists to see if the readerized content has
      // become available.
      ReadabilityService.sharedInstance.process(
        readerModeUrl,
        cache: ReaderModeHandler.readerModeCache,
        braveCore: braveCore
      )
      if let asset = Bundle.module.url(forResource: "ReaderViewLoading", withExtension: "html"),
        var contents = await AsyncFileManager.default.utf8Contents(at: asset)
      {
        let mapping = [
          "%ORIGINAL-URL%": readerModeUrl.absoluteString,
          "%READER-URL%": url.url.absoluteString,
          "%LOADING-TEXT%": Strings.readerModeLoadingContentDisplayText,
          "%LOADING-FAILED-TEXT%": Strings.readerModePageCantShowDisplayText,
          "%LOAD-ORIGINAL-TEXT%": Strings.readerModeLoadOriginalLinkText,
        ]

        mapping.forEach {
          contents = contents.replacingOccurrences(of: $0.key, with: $0.value)
        }

        guard
          let response = HTTPURLResponse(
            url: url.url,
            statusCode: 200,
            httpVersion: "HTTP/1.1",
            headerFields: ["Content-Type": "text/html; charset=UTF-8"]
          )
        else {
          return nil
        }

        let data = Data(contents.utf8)
        return (response, data)
      }
    }

    assert(false)
    return nil
  }
}
