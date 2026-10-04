// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

public protocol AIChatWebDelegate: AnyObject {
  var title: String? { get }
  var url: URL? { get }
  var isLoading: Bool { get }

  @MainActor
  func getPageContentType() async -> String?

  @MainActor
  func getMainArticle() async -> String?

  @MainActor
  func getPDFDocument() async -> String?

  @MainActor
  func getPrintViewPDF() async -> Data?
}
