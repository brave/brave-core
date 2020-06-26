// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

@available(iOS 13.0, *)
extension UIAction {
    /// An action handler to use for a UIAction when you need to wait until the context menu finished the
    /// dismissal animation.
    ///
    /// This ensures that the user can:
    ///   1. See a change that happens based on their selection
    ///   2. Ensure that if the view which has shown the context menu were to be removed from the view
    ///      heirarchy as a result of the action, that the context menu does not crash on dismissal.
    static func deferredActionHandler(_ handler: @escaping UIActionHandler) -> UIActionHandler {
        return { action in
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
                handler(action)
            }
        }
    }
}
