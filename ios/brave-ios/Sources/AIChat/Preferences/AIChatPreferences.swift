// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public enum AIChat {
    public static let hasSeenIntro = Option<Bool>(
      key: "aichat.intro.hasBeenSeen", default: false)
    public static let subscriptionExpirationDate = Option<Date?>(
      key: "aichat.expiration-date", default: nil)
    public static let subscriptionOrderId = Option<String?>(
      key: "aichat.order-id", default: nil)
    public static let subscriptionHasCredentials = Option<Bool>(
      key: "aichat.credentials", default: false)
    public static let autocompleteSuggestionsEnabled = Option<Bool>(
      key: "aichat.autocompletesuggestions-enabled", default: true)
  }
}


public struct AIChatConstants {
  static let braveLeoWikiURL = URL(string: "https://github.com/brave/brave-browser/wiki/Brave-Leo")!
  static let braveLeoPrivacyPolicyURL = URL(string: "https://brave.com/privacy/browser/#brave-leo")!
}
