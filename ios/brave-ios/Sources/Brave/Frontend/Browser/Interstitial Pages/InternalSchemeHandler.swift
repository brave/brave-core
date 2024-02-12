// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShared

enum InternalPageSchemeHandlerError: Error {
  case badURL
  case noResponder
  case responderUnableToHandle
  case notAuthorized
}

public protocol InternalSchemeResponse {
  func response(forRequest: URLRequest) -> (URLResponse, Data)?
}

public class InternalSchemeHandler: NSObject, WKURLSchemeHandler {

  public static func response(forUrl url: URL) -> URLResponse {
    return URLResponse(url: url, mimeType: "text/html", expectedContentLength: -1, textEncodingName: "utf-8")
  }

  // Responders are looked up based on the path component, for instance responder["about/license"] is used for 'internal://local/about/license'
  public static var responders = [String: InternalSchemeResponse]()

  // Unprivileged internal:// urls might be internal resources in the app bundle ( i.e. <link href="errorpage-resource/NetError.css"> )
  func downloadResource(urlSchemeTask: WKURLSchemeTask) -> Bool {
    guard let url = urlSchemeTask.request.url else { return false }

    let allowedInternalResources = [
      // interstitial
      "/interstitial-style/InterstitialStyles.css": "text/css",
      "/interstitial-style/BlockedDomain.css": "text/css",
      "/interstitial-style/NetworkError.css": "text/css",
      "/interstitial-style/CertificateError.css": "text/css",
      "/interstitial-style/Web3Domain.css": "text/css",
      "/interstitial-style/IPFSPreference.css": "text/css",
      "/interstitial-icon/Generic.svg": "image/svg+xml",
      "/interstitial-icon/Cloud.svg": "image/svg+xml",
      "/interstitial-icon/Clock.svg": "image/svg+xml",
      "/interstitial-icon/Globe.svg": "image/svg+xml",
      "/interstitial-icon/Info.svg": "image/svg+xml",
      "/interstitial-icon/Warning.svg": "image/svg+xml",
      "/interstitial-icon/DarkWarning.svg": "image/svg+xml",
      "/interstitial-icon/Carret.png": "image/png",
      "/interstitial-icon/BraveIPFS.svg": "image/svg+xml",
      "/interstitial-icon/IPFSBackground.svg": "image/svg+xml",
      "/interstitial-icon/warning-triangle-outline.svg": "image/svg+xml",

      // readermode
      "/\(InternalURL.Path.readermode.rawValue)/styles/Reader.css": "text/css",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-Bold.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-BoldItalic.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-Book.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-Italic.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-Light.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-Medium.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-Regular.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-SemiBold.ttf": "application/x-font-ttf",
      "/\(InternalURL.Path.readermode.rawValue)/fonts/FiraSans-UltraLight.ttf": "application/x-font-ttf",
    ]

    // Handle resources from internal pages. For example 'internal://local/errorpage-resource/CertError.css'.
    if allowedInternalResources.contains(where: { url.path == $0.key }) {
      let path = url.lastPathComponent
      let mimeType = allowedInternalResources[url.path]

      if let res = Bundle.module.url(forResource: path, withExtension: nil),
        let data = try? Data(contentsOf: res) {
        urlSchemeTask.didReceive(URLResponse(url: url, mimeType: mimeType, expectedContentLength: -1, textEncodingName: nil))
        urlSchemeTask.didReceive(data)
        urlSchemeTask.didFinish()
        return true
      }
    }

    return false
  }

  public func webView(_ webView: WKWebView, start urlSchemeTask: WKURLSchemeTask) {
    guard let url = urlSchemeTask.request.url else {
      urlSchemeTask.didFailWithError(InternalPageSchemeHandlerError.badURL)
      return
    }
      
    let path = url.path.starts(with: "/") ? String(url.path.dropFirst()) : url.path

    // For non-main doc URL, try load it as a resource
    if !urlSchemeTask.request.isPrivileged, urlSchemeTask.request.mainDocumentURL != urlSchemeTask.request.url, downloadResource(urlSchemeTask: urlSchemeTask) {
      return
    }

    // Need a better way to detect when WebKit is making a request from interactionState vs. a regular request by the user
    // instead of having to check the cache policy
    if !urlSchemeTask.request.isPrivileged && urlSchemeTask.request.cachePolicy == .useProtocolCachePolicy {
      urlSchemeTask.didFailWithError(InternalPageSchemeHandlerError.notAuthorized)
      return
    }

    guard let responder = InternalSchemeHandler.responders[path] else {
      urlSchemeTask.didFailWithError(InternalPageSchemeHandlerError.noResponder)
      return
    }

    guard let (urlResponse, data) = responder.response(forRequest: urlSchemeTask.request) else {
      urlSchemeTask.didFailWithError(InternalPageSchemeHandlerError.responderUnableToHandle)
      return
    }

    urlSchemeTask.didReceive(urlResponse)
    urlSchemeTask.didReceive(data)
    urlSchemeTask.didFinish()
  }

  public func webView(_ webView: WKWebView, stop urlSchemeTask: WKURLSchemeTask) {}
}
