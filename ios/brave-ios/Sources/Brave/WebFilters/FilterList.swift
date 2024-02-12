// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

struct FilterList: Identifiable {
  /// This is the uuid of the default filter list. This is a special filter list which we use slim list content blockers for
  public static let defaultComponentUUID = "default"
  /// The component ID of the "Fanboy's Mobile Notifications List"
  /// This is a special filter list that is enabled by default
  public static let mobileAnnoyancesComponentID = "bfpgedeaaibpoidldhjcknekahbikncb"
  /// The component id of the cookie consent notices filter list.
  /// This is a special filter list that has more accessible UI to control it
  public static let cookieConsentNoticesComponentID = "cdbbhgbmjhfnhnmgeddbliobbofkgdhe"
  /// This is a list of disabled filter lists. These lists are disabled because they are incompatible with iOS (for the time being)
  public static let disabledComponentIDs = [
    // The Anti-porn list has 500251 rules and is strictly all content blocking driven content
    // The limit for the rule store is 150000 rules. We have no way to handle this at the current moment
    "lbnibkdpkdjnookgfeogjdanfenekmpe"
  ]

  var id: String { return entry.uuid }
  let order: Int
  let entry: AdblockFilterListCatalogEntry
  var isEnabled: Bool = false

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
    self.isEnabled = isEnabled ?? entry.defaultEnabled
  }
}
