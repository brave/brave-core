// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import UIKit

private class EmptyCollectionViewCell: UICollectionViewCell, CollectionViewReusable {
}

class FlexibleSpaceSectionProvider: NSObject, NTPSectionProvider {
  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(EmptyCollectionViewCell.self)
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return 1
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    return collectionView.dequeueReusableCell(for: indexPath) as EmptyCollectionViewCell
  }
}
