// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AppIntents

public struct OpenWebsiteIntent: AppIntent {
  public static var title: LocalizedStringResource = "Open Website"
  public static var openAppWhenRun = true

  @Parameter(title: "Website URL")
  public var websiteURL: String

  public init() {
    websiteURL = ""
  }

  public init(websiteURL: String) {
    self.websiteURL = websiteURL
  }

  public func perform() async throws -> some IntentResult {
    .result()
  }
}

public struct OpenHistoryWebsiteIntent: AppIntent {
  public static var title: LocalizedStringResource = "Open Website from History"
  public static var openAppWhenRun = true

  @Parameter(title: "Website URL")
  public var websiteURL: String

  public init() {
    websiteURL = ""
  }

  public init(websiteURL: String) {
    self.websiteURL = websiteURL
  }

  public func perform() async throws -> some IntentResult {
    .result()
  }
}

public struct OpenBookmarkWebsiteIntent: AppIntent {
  public static var title: LocalizedStringResource = "Open Bookmark"
  public static var openAppWhenRun = true

  @Parameter(title: "Website URL")
  public var websiteURL: String

  public init() {
    websiteURL = ""
  }

  public init(websiteURL: String) {
    self.websiteURL = websiteURL
  }

  public func perform() async throws -> some IntentResult {
    .result()
  }
}
