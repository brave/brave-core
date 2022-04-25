// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

struct PrivacyReportsWebsite: Identifiable {
  let domain: String
  /// Due to our Favicon Fetcher logic we have to store the favicon url in certain format in order to fetch it properly.
  let faviconUrl: String
  let count: Int
  
  var id: String {
    domain
  }
}
