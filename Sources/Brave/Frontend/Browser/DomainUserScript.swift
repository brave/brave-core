// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShields
import WebKit

enum DomainUserScript: CaseIterable {
  case braveSearchHelper
#if canImport(BraveTalk)
  case braveTalkHelper
#endif
  case braveSkus
  case bravePlaylistFolderSharingHelper

  /// Initialize this script with a URL
  init?(for url: URL) {
    // First we look for an exact domain match
    if let host = url.host, let found = Self.allCases.first(where: { $0.associatedDomains.contains(host) }) {
      self = found
      return
    }

    // If no matches, we look for a baseDomain (eTLD+1) match.
    if let baseDomain = url.baseDomain, let found = Self.allCases.first(where: { $0.associatedDomains.contains(baseDomain) }) {
      self = found
      return
    }

    return nil
  }

  /// The domains associated with this script.
  var associatedDomains: Set<String> {
    switch self {
    case .braveSearchHelper:
      return Set(["search.brave.com", "search.brave.software",
                  "search.bravesoftware.com", "safesearch.brave.com",
                  "safesearch.brave.software", "safesearch.bravesoftware.com",
                  "search-dev-local.brave.com"])
#if canImport(BraveTalk)
    case .braveTalkHelper:
      return Set(["talk.brave.com", "beta.talk.brave.com",
                 "talk.bravesoftware.com", "beta.talk.bravesoftware.com",
                 "dev.talk.brave.software", "beta.talk.brave.software",
                 "talk.brave.software"])
#endif
    case .bravePlaylistFolderSharingHelper:
      return Set(["playlist.bravesoftware.com", "playlist.brave.com"])
    case .braveSkus:
      return Set(["account.brave.com",
                   "account.bravesoftware.com",
                   "account.brave.software"])
    }
  }
  
  /// Returns a shield type for a given user script domain.
  /// Returns nil if the domain's user script can't be turned off via a shield toggle. (i.e. it's always enabled)
  var requiredShield: BraveShield? {
    switch self {
    case .braveSearchHelper, .bravePlaylistFolderSharingHelper, .braveSkus:
      return nil
#if canImport(BraveTalk)
    case .braveTalkHelper:
      return nil
#endif
    }
  }
}
