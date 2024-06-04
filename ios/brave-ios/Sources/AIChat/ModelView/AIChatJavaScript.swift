// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

public protocol AIChatJavascript {
  @MainActor
  static func getPageContentType(webView: WKWebView) async -> String?

  @MainActor
  static func getMainArticle(webView: WKWebView) async -> String?

  @MainActor
  static func getPDFDocument(webView: WKWebView) async -> String?

  @MainActor
  static func getPrintViewPDF(webView: WKWebView) async -> Data
}

public protocol AIChatBraveTalkJavascript {
  @MainActor
  func getTranscript() async -> String?
}
