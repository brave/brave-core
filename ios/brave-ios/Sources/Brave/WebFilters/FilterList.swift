// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

struct FilterList: Identifiable {
  /// The component ID of the "Fanboy's Mobile Notifications List"
  /// This is a special filter list that is enabled by default
  public static let mobileAnnoyancesComponentID = "bfpgedeaaibpoidldhjcknekahbikncb"
  /// The component id of the cookie consent notices filter list.
  /// This is a special filter list that has more accessible UI to control it
  public static let cookieConsentNoticesComponentID = "cdbbhgbmjhfnhnmgeddbliobbofkgdhe"
  /// A list of safe filter lists that can be automatically enabled if the user has the matching localization.
  /// - Note: These are regional fiter lists that are well maintained. For now we hardcode these values
  /// but it would be better if our component updater told us which ones are safe in the future.
  public static let maintainedRegionalComponentIDs = [
    "llgjaaddopeckcifdceaaadmemagkepi" // Japanese filter lists
  ]
  /// This is a list of disabled filter lists. These lists are disabled because they are incompatible with iOS (for the time being)
  public static let disabledComponentIDs = [
    // The Anti-porn list has 500251 rules and is strictly all content blocking driven content
    // The limit for the rule store is 150000 rules. We have no way to handle this at the current moment
    "lbnibkdpkdjnookgfeogjdanfenekmpe"
  ]
  
  /// All the component ids that should be set to on by default.
  public static var defaultOnComponentIds: Set<String> {
    return [mobileAnnoyancesComponentID]
  }
  
  /// This is a list of component to UUID for some filter lists that have special toggles
  /// (which are availble before filter lists are downloaded)
  /// To save these values before filter lists are downloaded we need to also have the UUID
  public static var componentToUUID: [String: String] {
    return [
      mobileAnnoyancesComponentID: "2F3DCE16-A19A-493C-A88F-2E110FBD37D6",
      cookieConsentNoticesComponentID: "AC023D22-AE88-4060-A978-4FEEEC4221693"
    ]
  }
  
  var id: String { return entry.uuid }
  let order: Int
  let entry: AdblockFilterListCatalogEntry
  var isEnabled: Bool = false
  
  /// Tells us if this filter list is regional (i.e. if it contains language restrictions)
  var isRegional: Bool {
    return !entry.languages.isEmpty
  }
  
  /// Lets us know if this filter list is always aggressive.
  /// Aggressive filter lists are those that are non regional.
  var isAlwaysAggressive: Bool { !isRegional }
  
  init(from entry: AdblockFilterListCatalogEntry, order: Int, isEnabled: Bool) {
    self.entry = entry
    self.order = order
    self.isEnabled = isEnabled
  }
}
