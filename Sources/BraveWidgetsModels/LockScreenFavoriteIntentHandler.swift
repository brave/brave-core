// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Intents

public class LockScreenFavoriteIntentHandler: NSObject, LockScreenFavoriteConfigurationIntentHandling {
  public func provideFavoriteOptionsCollection(
    for intent: LockScreenFavoriteConfigurationIntent
  ) async throws -> INObjectCollection<FavoriteEntry> {
    let favs = FavoritesWidgetData.loadWidgetData() ?? []
    let entries: [FavoriteEntry] = favs.map { fav in
      let entry = FavoriteEntry(
        identifier: fav.url.absoluteString,
        display: fav.title ?? fav.url.absoluteString,
        subtitle: fav.title != nil ? fav.url.absoluteString : nil,
        image: nil
      )
      entry.url = fav.url
      return entry
    }
    return .init(items: entries)
  }
}
