// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Data
import Foundation
import Shared
import Storage
import UIKit

/// CoreData's diffable method relies on managed object ID only.
/// This does not work in our case since the object may be the same but with changed title or order.
/// This struct stores managed object ID as well as properties that we observer whether they have changed.
/// Note: `order` property does not have to be stored, favorite's frc handles order updates, it returns items in correct order.
struct FavoritesDiffable: Hashable {
  let objectID: NSManagedObjectID
  let title: String?
  let url: String?

  func hash(into hasher: inout Hasher) {
    hasher.combine(objectID)
  }
}

/// Favorites VC has two fetch result controllers to pull from.
/// This enum stores both models.
enum FavoritesDataWrapper: Hashable {
  case favorite(FavoritesDiffable)

  // Recent searches are static, we do not need any wrapper class for them.
  case recentSearch(NSManagedObjectID)
}

/// Favourites VC sections
enum FavoritesSection: Int, CaseIterable {
  case pasteboard
  case favorites
  case recentSearches
  case recentSearchesOptIn
}
