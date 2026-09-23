// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

/// A reactive grid layout that adjusts the number of columns and aspect ratio based on the
/// available size
class TabGridCollectionViewLayout: UICollectionViewCompositionalLayout {
  init() {
    super.init { _, layoutEnvironment in
      let containerSize = layoutEnvironment.container.effectiveContentSize
      let spacing: Double = 8
      let padding: Double = 16
      let numberOfColumns: Int = max(
        2,
        min(
          4,
          Int(floor((containerSize.width - (padding * 2)) / (175 + spacing)))
        )
      )
      let aspectRatio: Double = max(0.8, min(1.3, containerSize.height / containerSize.width))
      let item = NSCollectionLayoutItem(
        layoutSize: .init(
          widthDimension: .fractionalWidth(1.0 / Double(numberOfColumns)),
          heightDimension: .fractionalHeight(1)
        )
      )
      let group = NSCollectionLayoutGroup.horizontal(
        layoutSize: .init(
          widthDimension: .fractionalWidth(1.0),
          heightDimension: .fractionalWidth(aspectRatio / Double(numberOfColumns))
        ),
        subitems: [item]
      )
      group.interItemSpacing = .fixed(spacing)
      let section = NSCollectionLayoutSection(group: group)
      section.interGroupSpacing = spacing
      section.contentInsets = .init(
        top: 0,  // This is included in the content insets for the header toolbar
        leading: padding,
        bottom: padding,
        trailing: padding
      )
      return section
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func finalLayoutAttributesForDisappearingItem(
    at itemIndexPath: IndexPath
  ) -> UICollectionViewLayoutAttributes? {
    let attributes =
      super.finalLayoutAttributesForDisappearingItem(at: itemIndexPath)
      ?? .init(forCellWith: itemIndexPath)
    if let cell = collectionView?.cellForItem(at: itemIndexPath) as? TabGridItemCell,
      cell.model.swipeToDeleteGestureState != nil
    {
      // Ensure we keep the transform, center & alpha for cell being swiped away
      attributes.transform = cell.transform
      attributes.center.x = cell.center.x
      attributes.alpha = 1
      return attributes
    }
    // Small scale down animation applied to manually deleted items
    attributes.transform = .init(scaleX: 0.9, y: 0.9)
    return attributes
  }
}

extension UICollectionViewLayout {
  static var tabGridLayout: TabGridCollectionViewLayout {
    .init()
  }
}
