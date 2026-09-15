// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

struct FilterList: Identifiable, Equatable {
  var id: String { return entry.uuid }
  let order: Int
  let entry: AdblockFilterListCatalogEntry
  /// Whether the user has explicitly enabled or disabled this filter list.
  ///
  /// `nil` means the user has never made a choice for it.
  var isEnabled: Bool?

  /// The enabled state to use, falling back to the catalog default when the user
  /// has never made a choice for this filter list.
  var isEnabledOrDefault: Bool {
    return isEnabled ?? entry.defaultEnabled
  }

  var isHidden: Bool {
    return entry.hidden
  }

  /// Lets us know if this filter list is always aggressive.
  /// This value comes from `list_catalog.json` in brave core
  var engineType: GroupedAdBlockEngine.EngineType {
    return entry.firstPartyProtections ? .standard : .aggressive
  }

  init(from entry: AdblockFilterListCatalogEntry, order: Int, isEnabled: Bool?) {
    self.entry = entry
    self.order = order
    self.isEnabled = isEnabled
  }
}
