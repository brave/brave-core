// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

struct FilterList: Decodable, Identifiable {
  enum CodingKeys: String, CodingKey {
    case uuid, title, componentId, description = "desc", urlString = "url"
  }
  
  public static let mobileAnnoyancesComponentID = "ldbgldhcahahpffloggbbmjllggnkenk"
  /// The component id of the cookie consent notices filter list.
  /// This is a special filter list that has more accessible UI to control it
  public static let cookieConsentNoticesComponentID = "lfgnenkkneohplacnfabidofpgcdpofm"
  
  let uuid: String
  let title: String
  let description: String
  let componentId: String
  let urlString: String
  var isEnabled: Bool = false
  
  var id: String { return uuid }
  
  init(from filterList: AdblockFilterListCatalogEntry, isEnabled: Bool) {
    self.uuid = filterList.uuid
    self.title = filterList.title
    self.description = filterList.desc
    self.componentId = filterList.componentId
    self.isEnabled = isEnabled
    self.urlString = filterList.url
  }
  
  func makeRuleType() -> ContentBlockerManager.BlocklistRuleType {
    return .filterList(uuid: uuid)
  }
}
