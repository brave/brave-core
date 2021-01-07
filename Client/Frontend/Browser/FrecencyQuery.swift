// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Storage
import Data

class FrecencyQuery {
    private static let queue = DispatchQueue(label: "frecency-query-queue")
    private static var cancellable: DispatchWorkItem?
    
    public static func sitesByFrecency(containing query: String? = nil,
                                       completion: @escaping (Set<Site>) -> Void) {
        cancellable?.cancel()
        cancellable = nil
        
        // CoreData is fast, can be fetched on main thread. This also prevents threading problems.
        let historySites = History.byFrecency(query: query)
            .map { Site(url: $0.url ?? "", title: $0.title ?? "") }
        cancellable = DispatchWorkItem {
            // brave-core fetch can be slow over 200ms per call,
            // a cancellable serial queue is used for it.
            let bookmarkSites = Bookmarkv2.byFrequency(query: query)
                .map { Site(url: $0.url ?? "", title: $0.title ?? "", bookmarked: true) }
            
            let result = Set<Site>(historySites + bookmarkSites)
            
            DispatchQueue.main.async {
                completion(result)
            }
        }
        
        if let task = cancellable {
            queue.async(execute: task)
        }
    }
}
