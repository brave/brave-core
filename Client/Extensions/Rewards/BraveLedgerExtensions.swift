// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards

extension BraveLedger {
    
    public func listAutoContributePublishers(_ completion: @escaping (_ publishers: [PublisherInfo]) -> Void) {
        let filter: ActivityInfoFilter = {
            let sort = ActivityInfoFilterOrderPair().then {
                $0.propertyName = "percent"
                $0.ascending = false
            }
            let filter = ActivityInfoFilter().then {
                $0.id = ""
                $0.excluded = .filterAllExceptExcluded
                $0.percent = 1 //exclude 0% sites.
                $0.orderBy = [sort]
                $0.nonVerified = allowUnverifiedPublishers
                $0.reconcileStamp = autoContributeProperties.reconcileStamp
            }
            return filter
        }()
        listActivityInfo(fromStart: 0, limit: 0, filter: filter) { list in
            completion(list)
        }
    }
}
