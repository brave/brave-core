// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension AdblockFilterListCatalogEntry {
  /// The component ID of the "Default list"
  /// This is a special filter list that is enabled by default
  public static let defaultListComponentID = "iodkpdagapdfkphljnddpjlldadblomo"
  /// The component ID of the "Fanboy's Mobile Notifications List"
  /// This is a special filter list that is enabled by default
  public static let mobileAnnoyancesComponentID = "bfpgedeaaibpoidldhjcknekahbikncb"
  /// The component id of the cookie consent notices filter list.
  /// This is a special filter list that has more accessible UI to control it
  public static let cookieConsentNoticesComponentID = "cdbbhgbmjhfnhnmgeddbliobbofkgdhe"

  public static let disabledContentBlockersComponentIDs = [
    // The Anti-porn list has 500251 rules and is strictly all content blocking driven content
    // The limit for the rule store is 150000 rules. We have no way to handle this at the current moment
    "lbnibkdpkdjnookgfeogjdanfenekmpe",
    // For now we don't compile this into content blockers because we use the one coming from slim list
    // We might change this in the future as it ends up with 95k items whereas the limit is 150k.
    // So there is really no reason to use slim list except perhaps for performance which we need to test out.
    defaultListComponentID,
  ]

  /// Lets us know if this filter list is always aggressive.
  /// This value comes from `list_catalog.json` in brave core
  var engineType: GroupedAdBlockEngine.EngineType {
    return firstPartyProtections ? .standard : .aggressive
  }
}
