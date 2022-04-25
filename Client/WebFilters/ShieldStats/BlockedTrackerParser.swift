// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveShared

class BlockedTrackerParser {
  static let entityList = OnboardingDisconnectList.loadFromFile()

  static func parse(url: URL, fallbackToDomainURL: Bool) -> String? {
    guard let list = entityList else { return nil }

    let domain = url.baseDomain ?? url.host ?? url.schemelessAbsoluteString

    for entity in list.entities {
      let resources = entity.value.resources.filter({ $0 == domain })

      if !resources.isEmpty {
        return entity.key
      }
    }

    return fallbackToDomainURL ? domain : nil
  }
}
