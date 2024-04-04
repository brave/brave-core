// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension AdblockFilterListCatalogEntry {
  /// This is the uuid of the default filter list. This is a special filter list which we use slim list content blockers for
  public static let defaultFilterListComponentUUID = "default"
  /// The component ID of the "Fanboy's Mobile Notifications List"
  /// This is a special filter list that is enabled by default
  public static let mobileAnnoyancesComponentID = "bfpgedeaaibpoidldhjcknekahbikncb"
  /// The component id of the cookie consent notices filter list.
  /// This is a special filter list that has more accessible UI to control it
  public static let cookieConsentNoticesComponentID = "cdbbhgbmjhfnhnmgeddbliobbofkgdhe"

  public static let disabledFilterListComponentIDs = [
    // The Anti-porn list has 500251 rules and is strictly all content blocking driven content
    // The limit for the rule store is 150000 rules. We have no way to handle this at the current moment
    "lbnibkdpkdjnookgfeogjdanfenekmpe"
  ]

  /// Lets us know if this filter list is always aggressive.
  /// This value comes from `list_catalog.json` in brave core
  var engineType: GroupedAdBlockEngine.EngineType {
    return firstPartyProtections ? .standard : .aggressive
  }
}
