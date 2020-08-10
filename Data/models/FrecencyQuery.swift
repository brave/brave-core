// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Storage
import CoreData
import Shared

protocol WebsitePresentable {
    var title: String? { get }
    var url: String? { get }
}

protocol Frecencyable where Self: WebsitePresentable {
    static func byFrecency(query: String?, context: NSManagedObjectContext) -> [WebsitePresentable]
}

public class FrecencyQuery {
    // TODO: This is not a proper frecency query, it just gets sites from the past week. See issue #289
    public static func sitesByFrecency(containing query: String? = nil) -> Deferred<[Site]> {
        let result = Deferred<[Site]>()
        
        DataController.perform { context in
            let history = History.byFrecency(query: query, context: context)
            let bookmarks = Bookmark.byFrecency(query: query, context: context)
            
            // History must come before bookmarks, since later items replace existing ones, and want bookmarks to replace history entries
            let uniqueSites = Set<Site>((history + bookmarks).map { Site(url: $0.url ?? "", title: $0.title ?? "", bookmarked: $0 is Bookmark) })
            result.fill(Array(uniqueSites))
        }
        
        return result
    }
}
