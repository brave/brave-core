// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension URL {
  static let braveOriginCheckoutURL = URL(
    string: "https://account.brave.com/?intent=checkout&product=origin&mtm_campaign=iap-ios"
  )!
  static let braveOriginRefreshCredentials = URL(
    string:
      "https://account.brave.com/?intent=recover&product=origin&ux=mobile&mtm_campaign=refresh-ios"
  )!
  static let braveOriginLinkCredentials = URL(
    string: "https://account.brave.com/?intent=link-order&product=origin&mtm_campaign=linking-ios"
  )!
}
