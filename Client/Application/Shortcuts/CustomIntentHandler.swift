// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Intents
import CoreSpotlight
import MobileCoreServices

public class OpenWebsiteIntentHandler: NSObject, OpenWebsiteIntentHandling {

  public func handle(intent: OpenWebsiteIntent, completion: @escaping (OpenWebsiteIntentResponse) -> Void) {
    guard let siteURL = intent.websiteURL else {
      completion(OpenWebsiteIntentResponse(code: .failure, userActivity: nil))

      return
    }

    completion(OpenWebsiteIntentResponse.success(websiteURL: siteURL))
  }

  public func confirm(intent: OpenWebsiteIntent, completion: @escaping (OpenWebsiteIntentResponse) -> Void) {
    guard let urlString = intent.websiteURL, URL(string: urlString) != nil else {
      completion(OpenWebsiteIntentResponse(code: .failure, userActivity: nil))
      return
    }

    completion(OpenWebsiteIntentResponse(code: .success, userActivity: nil))
  }
}

public class OpenHistoryWebsiteIntentHandler: NSObject, OpenHistoryWebsiteIntentHandling {

  public func handle(intent: OpenHistoryWebsiteIntent, completion: @escaping (OpenHistoryWebsiteIntentResponse) -> Void) {
    guard let siteURL = intent.websiteURL else {
      completion(OpenHistoryWebsiteIntentResponse(code: .failure, userActivity: nil))

      return
    }

    completion(OpenHistoryWebsiteIntentResponse.success(websiteURL: siteURL))
  }

  public func confirm(intent: OpenHistoryWebsiteIntent, completion: @escaping (OpenHistoryWebsiteIntentResponse) -> Void) {
    guard let urlString = intent.websiteURL, URL(string: urlString) != nil else {
      completion(OpenHistoryWebsiteIntentResponse(code: .failure, userActivity: nil))
      return
    }

    completion(OpenHistoryWebsiteIntentResponse(code: .success, userActivity: nil))
  }
}

public class OpenBookmarkWebsiteIntentHandler: NSObject, OpenBookmarkWebsiteIntentHandling {

  public func handle(intent: OpenBookmarkWebsiteIntent, completion: @escaping (OpenBookmarkWebsiteIntentResponse) -> Void) {
    guard let siteURL = intent.websiteURL else {
      completion(OpenBookmarkWebsiteIntentResponse(code: .failure, userActivity: nil))

      return
    }

    completion(OpenBookmarkWebsiteIntentResponse.success(websiteURL: siteURL))
  }

  public func confirm(intent: OpenBookmarkWebsiteIntent, completion: @escaping (OpenBookmarkWebsiteIntentResponse) -> Void) {
    guard let urlString = intent.websiteURL, URL(string: urlString) != nil else {
      completion(OpenBookmarkWebsiteIntentResponse(code: .failure, userActivity: nil))
      return
    }

    completion(OpenBookmarkWebsiteIntentResponse(code: .success, userActivity: nil))
  }
}
