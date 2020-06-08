// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// The new tab page collection view layout
///
/// Handles correcting center-aligned single items in a flow layout while using
/// automatic sizing cells
class NewTabPageFlowLayout: UICollectionViewFlowLayout {
    override init() {
        super.init()
        estimatedItemSize = Self.automaticSize
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func layoutAttributesForItem(at indexPath: IndexPath) -> UICollectionViewLayoutAttributes? {
        guard let attribute = super.layoutAttributesForItem(at: indexPath)?.copy() as? UICollectionViewLayoutAttributes,
            let collectionView = collectionView else {
                return nil
        }

        if attribute.representedElementCategory != .cell {
            return attribute
        }

        // Left align the cells since they automatically center if there's only
        // 1 item in the section and use automaticSize...
        if estimatedItemSize == UICollectionViewFlowLayout.automaticSize {
            let indexPath = attribute.indexPath
            if collectionView.numberOfItems(inSection: indexPath.section) == 1 {
                // Obtain section inset/spacing to lay out each cell properly
                let sectionInset: UIEdgeInsets
                let minimumInteritemSpacing: CGFloat
                if let flowLayoutDelegate = collectionView.delegate as? UICollectionViewDelegateFlowLayout {
                    // If the layout has a delegate to obtain section specific
                    // info, grab that
                    sectionInset = flowLayoutDelegate.collectionView?(collectionView, layout: self, insetForSectionAt: indexPath.section) ?? self.sectionInset
                    minimumInteritemSpacing = flowLayoutDelegate.collectionView?(collectionView, layout: self, minimumInteritemSpacingForSectionAt: indexPath.section) ?? self.minimumInteritemSpacing
                } else {
                    // Otherwise default to the global values defined on the
                    // layout itself
                    sectionInset = self.sectionInset
                    minimumInteritemSpacing = self.minimumInteritemSpacing
                }
                // Layout the first item in the secton to far-left
                if attribute.indexPath.item == 0 {
                    attribute.frame.origin.x = sectionInset.left
                } else {
                    // Otherwise layout based on previous item's origin
                    if let previousItemAttribute = layoutAttributesForItem(at: IndexPath(item: indexPath.item - 1, section: indexPath.section)) {
                        attribute.frame.origin.x = previousItemAttribute.frame.maxX + minimumInteritemSpacing
                    }
                }
            }
        }

        return attribute
    }
    
    override func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
        guard let attributes = super.layoutAttributesForElements(in: rect) else {
                return nil
        }
        for attribute in attributes where attribute.representedElementCategory == .cell {
            if let frame = self.layoutAttributesForItem(at: attribute.indexPath)?.frame {
                attribute.frame = frame
            }
        }
        return attributes
    }
}
