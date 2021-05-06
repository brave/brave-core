// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared

extension UIViewController {
    func updateThemeForUserInterfaceStyleChange() {
        if Preferences.General.themeNormalMode.value != Theme.DefaultTheme.system.rawValue {
            // No need to do this if we're not using system themes
            return
        }
        
        // Have to wait until the appearance() methods are updated, which seem to be delayed
        // by a run loop execution, so we async on main
        DispatchQueue.main.async { [self] in
            // View manipulations done via `apperance()` do not impact existing UI, and require a
            // full view removal & re-add to apply the correct colors
            let superview = view.superview
            view.removeFromSuperview()
            superview?.addSubview(view)
            
            navigationController?.navigationBar.tintColor = UINavigationBar.appearance().tintColor
            navigationController?.navigationBar.barTintColor = UINavigationBar.appearance().appearanceBarTintColor
            navigationController?.navigationBar.titleTextAttributes = UINavigationBar.appearance().titleTextAttributes
            
            navigationController?.toolbar.tintColor = UINavigationBar.appearance().tintColor
            navigationController?.toolbar.barTintColor = UINavigationBar.appearance().appearanceBarTintColor
        }
    }
}
