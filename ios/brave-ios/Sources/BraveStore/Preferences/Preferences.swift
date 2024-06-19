// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public struct AIChat {
    /// The Order-ID of the user's current AI-Chat subscription
    public static let subscriptionOrderId = Option<String?>(
      key: "aichat.order-id",
      default: nil
    )

    public static let subscriptionProductId = Option<String?>(
      key: "aichat.subscription-product-id",
      default: nil
    )
  }
}
