// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreSpotlight
import Intents
import MobileCoreServices
import Shared

public class OpenWebsiteIntentHandler: NSObject, OpenWebsiteIntentHandling {

  public func handle(
    intent: OpenWebsiteIntent,
    completion: @escaping (OpenWebsiteIntentResponse) -> Void
  ) {
    guard let urlString = intent.websiteURL, let url = URL(string: urlString), let host = url.host,
      !host.isEmpty, url.isWebPage(includeDataURIs: false)
    else {
      completion(OpenWebsiteIntentResponse(code: .failure, userActivity: nil))

      return
    }

    completion(OpenWebsiteIntentResponse.success(websiteURL: urlString))
  }

  public func confirm(
    intent: OpenWebsiteIntent,
    completion: @escaping (OpenWebsiteIntentResponse) -> Void
  ) {
    guard let urlString = intent.websiteURL, let url = URL(string: urlString), let host = url.host,
      !host.isEmpty, url.isWebPage(includeDataURIs: false)
    else {
      completion(OpenWebsiteIntentResponse(code: .failure, userActivity: nil))
      return
    }

    completion(OpenWebsiteIntentResponse(code: .success, userActivity: nil))
  }
}

public class OpenHistoryWebsiteIntentHandler: NSObject, OpenHistoryWebsiteIntentHandling {

  public func handle(
    intent: OpenHistoryWebsiteIntent,
    completion: @escaping (OpenHistoryWebsiteIntentResponse) -> Void
  ) {
    guard let urlString = intent.websiteURL, let url = URL(string: urlString), let host = url.host,
      !host.isEmpty, url.isWebPage(includeDataURIs: false)
    else {
      completion(OpenHistoryWebsiteIntentResponse(code: .failure, userActivity: nil))

      return
    }

    completion(OpenHistoryWebsiteIntentResponse.success(websiteURL: urlString))
  }

  public func confirm(
    intent: OpenHistoryWebsiteIntent,
    completion: @escaping (OpenHistoryWebsiteIntentResponse) -> Void
  ) {
    guard let urlString = intent.websiteURL, let url = URL(string: urlString), let host = url.host,
      !host.isEmpty, url.isWebPage(includeDataURIs: false)
    else {
      completion(OpenHistoryWebsiteIntentResponse(code: .failure, userActivity: nil))
      return
    }

    completion(OpenHistoryWebsiteIntentResponse(code: .success, userActivity: nil))
  }
}

public class OpenBookmarkWebsiteIntentHandler: NSObject, OpenBookmarkWebsiteIntentHandling {

  public func handle(
    intent: OpenBookmarkWebsiteIntent,
    completion: @escaping (OpenBookmarkWebsiteIntentResponse) -> Void
  ) {
    guard let urlString = intent.websiteURL, let url = URL(string: urlString), let host = url.host,
      !host.isEmpty, url.isWebPage(includeDataURIs: false)
    else {
      completion(OpenBookmarkWebsiteIntentResponse(code: .failure, userActivity: nil))

      return
    }

    completion(OpenBookmarkWebsiteIntentResponse.success(websiteURL: urlString))
  }

  public func confirm(
    intent: OpenBookmarkWebsiteIntent,
    completion: @escaping (OpenBookmarkWebsiteIntentResponse) -> Void
  ) {
    guard let urlString = intent.websiteURL, let url = URL(string: urlString), let host = url.host,
      !host.isEmpty, url.isWebPage(includeDataURIs: false)
    else {
      completion(OpenBookmarkWebsiteIntentResponse(code: .failure, userActivity: nil))
      return
    }

    completion(OpenBookmarkWebsiteIntentResponse(code: .success, userActivity: nil))
  }
}
