// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

public protocol AIChatJavascript {
  @MainActor
  static func getPageContentType(webView: CWVWebView) async -> String?

  @MainActor
  static func getMainArticle(webView: CWVWebView) async -> String?

  @MainActor
  static func getPDFDocument(webView: CWVWebView) async -> String?

  @MainActor
  static func getPrintViewPDF(webView: CWVWebView) async -> Data
}

public protocol AIChatBraveTalkJavascript {
  @MainActor
  func getTranscript() async -> String?
}
