// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI

class BraveTodaySectionProvider: NSObject, NTPObservableSectionProvider {
    let dataSource: FeedDataSource
    var sectionDidChange: (() -> Void)?
    
    init(dataSource: FeedDataSource) {
        self.dataSource = dataSource
        super.init()
        
        self.dataSource.load { [weak self] in
            self?.sectionDidChange?()
        }
    }
    
    func registerCells(to collectionView: UICollectionView) {
        collectionView.register(FeedCardCell<VerticalFeedGroupView>.self)
        collectionView.register(FeedCardCell<NumberedFeedGroupView>.self)
    }
    
    var landscapeBehavior: NTPLandscapeSizingBehavior {
        .fullWidth
    }
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return dataSource.cards.count
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        return UICollectionViewCell()
    }
}
