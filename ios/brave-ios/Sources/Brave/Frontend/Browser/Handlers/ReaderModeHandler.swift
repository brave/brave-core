// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

public class ReaderModeHandler: InternalSchemeResponse {
  private let profile: Profile
  public static let path = InternalURL.Path.readermode.rawValue
  internal static var readerModeCache: ReaderModeCache = DiskReaderModeCache.sharedInstance
  private static let readerModeStyleHash = "sha256-L2W8+0446ay9/L1oMrgucknQXag570zwgQrHwE68qbQ="
  
  public init(profile: Profile) {
    self.profile = profile
  }

  public func response(forRequest request: URLRequest) -> (URLResponse, Data)? {
    guard let _url = request.url,
          let url = InternalURL(_url),
          let readerModeUrl = url.extractedUrlParam else {
      return nil
    }
    
    if url.url.lastPathComponent == "page-exists" {
      let statusCode = ReaderModeHandler.readerModeCache.contains(readerModeUrl) ? 200 : 400
      if let response = HTTPURLResponse(url: url.url,
                                        statusCode: statusCode,
                                        httpVersion: "HTTP/1.1",
                                        headerFields: ["Content-Type": "text/html; charset=UTF-8"]) {
        return (response, Data())
      }
      return nil
    }
    
    // From here on, handle 'url=' query param
    if !readerModeUrl.isSecureWebPage() {
      if let response = HTTPURLResponse(url: url.url,
                                        statusCode: 500,
                                        httpVersion: "HTTP/1.1",
                                        headerFields: ["Content-Type": "text/html; charset=UTF-8"]) {
        return (response, Data())
      }
      return nil
    }
    
    do {
      let readabilityResult = try ReaderModeHandler.readerModeCache.get(readerModeUrl)
      // We have this page in our cache, so we can display it. Just grab the correct style from the
      // profile and then generate HTML from the Readability results.
      var readerModeStyle = DefaultReaderModeStyle
      if let dict = profile.prefs.dictionaryForKey(ReaderModeProfileKeyStyle) {
        if let style = ReaderModeStyle(dict: dict) {
          readerModeStyle = style
        }
      }

      // Must generate a unique nonce, every single time as per Content-Policy spec.
      let setTitleNonce = UUID().uuidString.replacingOccurrences(of: "-", with: "")

      if let html = ReaderModeUtils.generateReaderContent(
        readabilityResult, initialStyle: readerModeStyle,
        titleNonce: setTitleNonce),
         let data = html.data(using: .utf8) {
        // Apply a Content Security Policy that disallows everything except images from anywhere and fonts and css from our internal server
        
        if let response = HTTPURLResponse(url: url.url,
                                       statusCode: 200,
                                       httpVersion: "HTTP/1.1",
                                       headerFields: ["Content-Type": "text/html; charset=UTF-8",
                                                      "Content-Security-Policy": "default-src 'none'; img-src *; style-src \(InternalURL.baseUrl) '\(ReaderModeHandler.readerModeStyleHash)'; font-src \(InternalURL.baseUrl); script-src 'nonce-\(setTitleNonce)'"]) {
          return (response, data)
        }
        return nil
      }
    } catch {
      // This page has not been converted to reader mode yet. This happens when you for example add an
      // item via the app extension and the application has not yet had a change to readerize that
      // page in the background.
      //
      // What we do is simply queue the page in the ReadabilityService and then show our loading
      // screen, which will periodically call page-exists to see if the readerized content has
      // become available.
      ReadabilityService.sharedInstance.process(readerModeUrl, cache: ReaderModeHandler.readerModeCache)
      if let readerViewLoadingPath = Bundle.module.path(forResource: "ReaderViewLoading", ofType: "html") {
        do {
          var contents = try String(contentsOfFile: readerViewLoadingPath)
          let mapping = [
            "%ORIGINAL-URL%": readerModeUrl.absoluteString,
            "%READER-URL%": url.url.absoluteString,
            "%LOADING-TEXT%": Strings.readerModeLoadingContentDisplayText,
            "%LOADING-FAILED-TEXT%": Strings.readerModePageCantShowDisplayText,
            "%LOAD-ORIGINAL-TEXT%": Strings.readerModeLoadOriginalLinkText
          ]
          
          mapping.forEach {
            contents = contents.replacingOccurrences(of: $0.key, with: $0.value)
          }
          
          if let response = HTTPURLResponse(url: url.url,
                                         statusCode: 200,
                                         httpVersion: "HTTP/1.1",
                                         headerFields: ["Content-Type": "text/html; charset=UTF-8"]),
             let data = contents.data(using: .utf8) {
            return (response, data)
          }
          return nil
        } catch {
          assertionFailure("CANNOT LOAD  ReaderViewLoading.html: \(error)")
        }
      }
    }
    
    assert(false)
    return nil
  }
}
