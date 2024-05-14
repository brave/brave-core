// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public enum AIChat {
    /// A boolean indicating whether the user has seen the AI-Chat intro screen at least once
    public static let hasSeenIntro = Option<Bool>(
      key: "aichat.intro.hasBeenSeen",
      default: false
    )

    /// The date the user's current AI-Chat subscription expires
    public static let subscriptionExpirationDate = Option<Date?>(
      key: "aichat.expiration-date",
      default: nil
    )

    /// The Order-ID of the user's current AI-Chat subscription
    public static let subscriptionOrderId = Option<String?>(
      key: "aichat.order-id",
      default: nil
    )

    public static let subscriptionProductId = Option<String?>(
      key: "aichat.subscription-product-id",
      default: nil
    )

    /// A boolean indicating whether or not the user has URL-Bar/Search-Bar auto-complete for AI-Chat
    public static let autocompleteSuggestionsEnabled = Option<Bool>(
      key: "aichat.autocompletesuggestions-enabled",
      default: true
    )

    /// A boolean indicating whether or not the user has dismissed the Premium Prompt on the Feedback Form
    public static let showPremiumFeedbackAd = Option<Bool>(
      key: "aichat.show-premium-feedback-ad",
      default: true
    )
  }
}

public struct AIChatConstants {
  static let braveLeoWikiURL = URL(string: "https://github.com/brave/brave-browser/wiki/Brave-Leo")!
  static let braveLeoPrivacyPolicyURL = URL(string: "https://brave.com/privacy/browser/#brave-leo")!
}
