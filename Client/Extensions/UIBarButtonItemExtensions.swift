/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

extension UIBarButtonItem {
    
    /// Creates a fixed space `UIBarButtonItem` with a given width
    class func fixedSpace(_ width: CGFloat) -> UIBarButtonItem {
        return UIBarButtonItem(barButtonSystemItem: .fixedSpace, target: nil, action: nil).then {
            $0.width = width
        }
    }
}
