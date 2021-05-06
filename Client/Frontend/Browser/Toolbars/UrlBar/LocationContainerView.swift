// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

// We need a subclass so we can setup the shadows correctly
// This subclass creates a strong shadow on the URLBar
class LocationContainerView: UIView {
    
    private struct LocationContainerUX {
        static let cornerRadius: CGFloat = 8.0
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        let layer = self.layer
        layer.cornerRadius = LocationContainerUX.cornerRadius
        layer.cornerCurve = .continuous
        layer.masksToBounds = true
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
