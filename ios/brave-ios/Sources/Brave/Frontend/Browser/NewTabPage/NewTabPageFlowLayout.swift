// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

/// The new tab page collection view layout
///
/// Handles correcting center-aligned single items in a flow layout while using
/// automatic sizing cells
class NewTabPageFlowLayout: UICollectionViewFlowLayout {
  /// Brave News section acts a little differently, as it is pushed to the bottom of the screen despite
  /// there being space between, therefore additional space has to be given to the overall content size
  /// when Brave News is enabled
  var braveNewsSection: Int? {
    didSet {
      invalidateLayout()
    }
  }

  override init() {
    super.init()
    estimatedItemSize = Self.automaticSize
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private var gapLength: CGFloat = 0.0
  private var extraHeight: CGFloat = 0.0
  private let gapPadding: CGFloat = 32.0

  override func prepare() {
    super.prepare()
    if let braveNewsSection = braveNewsSection,
      let collectionView = collectionView,
      collectionView.numberOfItems(inSection: braveNewsSection) != 0,
      let attribute = super.layoutAttributesForItem(
        at: IndexPath(item: 0, section: braveNewsSection)
      )
    {
      let diff = collectionView.frame.height - attribute.frame.minY
      gapLength = diff - gapPadding

      // Obtain the total height of the Brave News section to calculate any extra height to be added
      // to the content size. The extra height will ensure that there is always enough space to scroll
      // the header into full-visibility
      let numberOfItems = collectionView.numberOfItems(inSection: braveNewsSection)
      if let lastItemAttribute = super.layoutAttributesForItem(
        at: IndexPath(item: numberOfItems - 1, section: braveNewsSection)
      ) {
        if lastItemAttribute.frame.maxY - attribute.frame.minY < collectionView.bounds.height
          - gapPadding
        {
          extraHeight =
            (collectionView.bounds.height - gapPadding)
            - (lastItemAttribute.frame.maxY - attribute.frame.minY)
        }
      }

      lastSizedElementMinY = nil
      lastSizedElementPreferredHeight = nil
    } else {
      gapLength = 0
      extraHeight = 0
    }
  }

  override func layoutAttributesForItem(
    at indexPath: IndexPath
  ) -> UICollectionViewLayoutAttributes? {
    guard
      let attribute = super.layoutAttributesForItem(at: indexPath)?.copy()
        as? UICollectionViewLayoutAttributes,
      let collectionView = collectionView
    else {
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
          sectionInset =
            flowLayoutDelegate.collectionView?(
              collectionView,
              layout: self,
              insetForSectionAt: indexPath.section
            ) ?? self.sectionInset
          minimumInteritemSpacing =
            flowLayoutDelegate.collectionView?(
              collectionView,
              layout: self,
              minimumInteritemSpacingForSectionAt: indexPath.section
            ) ?? self.minimumInteritemSpacing
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
          if let previousItemAttribute = layoutAttributesForItem(
            at: IndexPath(item: indexPath.item - 1, section: indexPath.section)
          ) {
            attribute.frame.origin.x = previousItemAttribute.frame.maxX + minimumInteritemSpacing
          }
        }
      }
    }

    if let section = braveNewsSection, indexPath.section == section {
      attribute.frame.origin.y += gapLength
    }

    return attribute
  }

  override func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]?
  {
    var adjustedRect = rect
    adjustedRect.origin.y -= gapLength
    adjustedRect.size.height += gapLength * 2
    guard let attributes = super.layoutAttributesForElements(in: adjustedRect) else {
      return nil
    }
    for attribute in attributes where attribute.representedElementCategory == .cell {
      if let frame = self.layoutAttributesForItem(at: attribute.indexPath)?.frame {
        attribute.frame = frame
      }
    }
    return attributes
  }

  override var collectionViewContentSize: CGSize {
    var size = super.collectionViewContentSize
    if braveNewsSection != nil {
      size.height += gapLength + extraHeight
    }
    return size
  }

  override func targetContentOffset(
    forProposedContentOffset proposedContentOffset: CGPoint,
    withScrollingVelocity velocity: CGPoint
  ) -> CGPoint {
    guard let section = braveNewsSection,
      collectionView?.numberOfItems(inSection: section) != 0,
      let item = layoutAttributesForItem(at: IndexPath(item: 0, section: section))
    else {
      return proposedContentOffset
    }
    var offset = proposedContentOffset
    let flicked = abs(velocity.y) > 0.3
    if (offset.y > item.frame.minY / 2 && offset.y < item.frame.minY)
      || (flicked && velocity.y > 0 && offset.y < item.frame.minY)
    {
      offset.y = item.frame.minY - 56  // FIXME: Use size of header + padding
    } else if offset.y < item.frame.minY {
      offset.y = 0
    }
    return offset
  }

  override func shouldInvalidateLayout(
    forPreferredLayoutAttributes preferredAttributes: UICollectionViewLayoutAttributes,
    withOriginalAttributes originalAttributes: UICollectionViewLayoutAttributes
  ) -> Bool {
    if let section = braveNewsSection,
      preferredAttributes.representedElementCategory == .cell,
      preferredAttributes.indexPath.section == section
    {
      return preferredAttributes.size.height.rounded() != originalAttributes.size.height.rounded()
    }
    return super.shouldInvalidateLayout(
      forPreferredLayoutAttributes: preferredAttributes,
      withOriginalAttributes: originalAttributes
    )
  }

  override func invalidationContext(
    forPreferredLayoutAttributes preferredAttributes: UICollectionViewLayoutAttributes,
    withOriginalAttributes originalAttributes: UICollectionViewLayoutAttributes
  ) -> UICollectionViewLayoutInvalidationContext {
    let context = super.invalidationContext(
      forPreferredLayoutAttributes: preferredAttributes,
      withOriginalAttributes: originalAttributes
    )

    guard let collectionView = collectionView, let _ = braveNewsSection else {
      return context
    }

    // Big thanks to Bryan Keller for the solution found in this `airbnb/MagazineLayout` PR:
    // https://github.com/airbnb/MagazineLayout/pull/11/files
    //
    // Original comment:
    // If layout information is discarded above our current scroll position (on rotation, for
    // example), we need to compensate for preferred size changes to items as we're scrolling up,
    // otherwise, the collection view will appear to jump each time an element is sized.
    // Since size adjustments can occur for multiple items in the same soon-to-be-visible row, we
    // need to account for this by considering the preferred height for previously sized elements in
    // the same row so that we only adjust the content offset by the exact amount needed to create
    // smooth scrolling.
    let currentElementY = originalAttributes.frame.minY
    let isScrolling = collectionView.isDragging || collectionView.isDecelerating
    let isSizingElementAboveTopEdge = originalAttributes.frame.minY < collectionView.contentOffset.y

    if isScrolling && isSizingElementAboveTopEdge {
      let isSameRowAsLastSizedElement = lastSizedElementMinY == currentElementY
      if isSameRowAsLastSizedElement {
        let lastSizedElementPreferredHeight = self.lastSizedElementPreferredHeight ?? 0
        if preferredAttributes.size.height > lastSizedElementPreferredHeight {
          context.contentOffsetAdjustment.y =
            preferredAttributes.size.height - lastSizedElementPreferredHeight
        }
      } else {
        context.contentOffsetAdjustment.y =
          preferredAttributes.size.height - originalAttributes.size.height
      }
    }

    lastSizedElementMinY = currentElementY
    lastSizedElementPreferredHeight = preferredAttributes.size.height

    return context
  }

  private var lastSizedElementMinY: CGFloat?
  private var lastSizedElementPreferredHeight: CGFloat?
}
