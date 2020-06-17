// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Storage
import BraveUI

/// The actions you can perform on a feed item in the list
enum FeedItemAction: Equatable {
    /// The user simply tapped on the item and should open the feed URL
    case tapped
    /// The user choose to open the feed item in a new tab (and possibly in private mode)
    case openInNewTab(_ isPrivate: Bool = false)
}

protocol FeedCardContent {
    var view: UIView { get }
    var actionHandler: ((_ itemIndex: Int, _ action: FeedItemAction) -> Void)? { get set }
    init()
}

extension FeedCardContent where Self: UIView {
    var view: UIView { self }
}

/// Defines the basic feed card cell. A feed card can display 1 or more feed
/// items. This cell is defined by the `View` type
class FeedCardCell<Content: FeedCardContent>: UICollectionViewCell, CollectionViewReusable {
    var content = Content()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        contentView.addSubview(content.view)
        content.view.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
        // swiftlint:disable:next force_cast
        let attributes = layoutAttributes.copy() as! UICollectionViewLayoutAttributes
        attributes.size = systemLayoutSizeFitting(
            UIView.layoutFittingCompressedSize,
            withHorizontalFittingPriority: .required,
            verticalFittingPriority: .fittingSizeLevel
        )
        return attributes
    }
}
