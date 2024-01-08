// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import BraveShared
import Favicon
import BraveWidgetsModels

extension BrowserViewController: NSFetchedResultsControllerDelegate {
  public func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    updateWidgetFavoritesData()
  }

  func updateWidgetFavoritesData() {
    guard let frc = widgetBookmarksFRC else { return }
    try? frc.performFetch()

    if let favs = frc.fetchedObjects {
      Task { @MainActor in
        struct IndexedWidgetFavorite {
          let index: Int
          let favorite: WidgetFavorite
        }

        let widgets = await withTaskGroup(of: IndexedWidgetFavorite.self, returning: [WidgetFavorite].self) { group in
          // MUST copy `favs` array.
          // If we don't, operating on `favs` is undefined behaviour on iOS 14! - Brandon T.
          // This causes a terribly difficult to find bug on iOS 14 (dangling pointer).
          // See also: https://bugs.swift.org/browse/SR-12288
          for (index, fav) in Array(favs).prefix(16).enumerated() {
            guard let url = fav.url?.asURL else {
              continue
            }
            
            let title = fav.title
            
            group.addTask {
              if let favicon = FaviconFetcher.getIconFromCache(for: url) {
                return IndexedWidgetFavorite(index: index, favorite: .init(url: url, title: title, favicon: favicon))
              }
              
              let favicon = try? await FaviconFetcher.loadIcon(url: url, kind: .largeIcon, persistent: true)
              return IndexedWidgetFavorite(index: index, favorite: .init(url: url, title: title, favicon: favicon))
            }
          }
          
          var results = [IndexedWidgetFavorite]()
          for await result in group {
            results.append(result)
          }
          return results.sorted { $0.index < $1.index }.map { $0.favorite }
        }
        
        FavoritesWidgetData.updateWidgetData(widgets)
      }
    }
  }
}
