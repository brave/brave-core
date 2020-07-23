// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Storage
import BraveUI

/// Defines the basic feed card cell. A feed card can display 1 or more feed
/// items. This cell is defined by the `View` type
class FeedCardCell<View: UIView>: UICollectionViewCell, CollectionViewReusable {
    var view = View()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        contentView.addSubview(view)
        view.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
