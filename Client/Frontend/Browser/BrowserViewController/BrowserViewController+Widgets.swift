// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import BraveShared

extension BrowserViewController: NSFetchedResultsControllerDelegate {
    func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        updateWidgetFavoritesData()
    }
    
    func updateWidgetFavoritesData() {
        guard let frc = widgetBookmarksFRC else { return }
        try? frc.performFetch()
        
        if let favs = frc.fetchedObjects {
            let group = DispatchGroup()
            var favData: [Int: WidgetFavorite] = [:]
            
            // MUST copy `favs` array.
            // If we don't, operating on `favs` is undefined behaviour on iOS 14! - Brandon T.
            // This causes a terribly difficult to find bug on iOS 14 (dangling pointer).
            // See also: https://bugs.swift.org/browse/SR-12288
            for (index, fav) in Array(favs).prefix(16).enumerated() {
                if let url = fav.url?.asURL {
                    group.enter()
                    let fetcher = FaviconFetcher(siteURL: url, kind: .largeIcon, persistentCheckOverride: true)
                    widgetFaviconFetchers.append(fetcher)
                    fetcher.load { _, attributes in
                        favData[index] = .init(url: url, favicon: attributes)
                        group.leave()
                    }
                }
            }

            group.notify(queue: .main) { [self] in
                widgetFaviconFetchers.removeAll()
                // While we get favorites from the database in correct order,
                // filling it with favicon data is an async operation.
                // To preserve favorites order
                // we add index number to each item then use it to sort it back.
                let sortedData = favData
                    .sorted { $0.key < $1.key }
                    .map { $0.value }

                FavoritesWidgetData.updateWidgetData(sortedData)
            }
        }
    }
}
