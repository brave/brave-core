// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Storage
import Data

class FrequencyQuery {
    
    private static let queue = DispatchQueue(label: "frequency-query-queue")
    private static var cancellable: DispatchWorkItem?
    
    public static func sitesByFrequency(containing query: String? = nil,
                                        completion: @escaping (Set<Site>) -> Void) {
        
        Historyv2.byFrequency(query: query) { historyList in
            let historySites = historyList
                .map { Site(url: $0.url ?? "", title: $0.title ?? "") }
            
            cancellable = DispatchWorkItem {
                // brave-core fetch can be slow over 200ms per call,
                // a cancellable serial queue is used for it.
                DispatchQueue.main.async {
                    let bookmarkSites = Bookmarkv2.byFrequency(query: query)
                        .map { Site(url: $0.url ?? "", title: $0.title ?? "", bookmarked: true) }

                    let result = Set<Site>(historySites + bookmarkSites)

                    completion(result)
                }
            }
        }
        
        if let task = cancellable {
            queue.async(execute: task)
        }
    }
}
