// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

func shouldBlockUniversalLinksFor(request: URLRequest, isPrivateBrowsing: Bool) -> Bool {
  func isYouTubeLoad() -> Bool {
    guard let domain = request.mainDocumentURL?.baseDomain else {
      return false
    }
    let domainsWithUniversalLinks: Set<String> = ["youtube.com", "youtu.be"]
    return domainsWithUniversalLinks.contains(domain)
  }
  if isPrivateBrowsing || !Preferences.General.followUniversalLinks.value
    || (Preferences.General.keepYouTubeInBrave.value && isYouTubeLoad())
  {
    return true
  }
  return false
}
