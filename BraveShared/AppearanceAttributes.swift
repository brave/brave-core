// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

public extension UILabel {
    @objc dynamic var appearanceTextColor: UIColor! {
        get { return self.textColor }
        set { self.textColor = newValue }
    }
}

public extension UITableView {
    @objc dynamic var appearanceSeparatorColor: UIColor? {
        get { return self.separatorColor }
        set { self.separatorColor = newValue }
    }
}

public extension UITextView {
    @objc dynamic var appearanceTextColor: UIColor? {
        get { return self.textColor }
        set { self.textColor = newValue }
    }
}

public extension UIView {
    @objc dynamic var appearanceBackgroundColor: UIColor? {
        get { return self.backgroundColor }
        set { self.backgroundColor = newValue }
    }
}

public extension UITextField {
    @objc dynamic var appearanceTextColor: UIColor? {
        get { return self.textColor }
        set { self.textColor = newValue }
    }
}

public extension UIView {
    @objc dynamic var appearanceOverrideUserInterfaceStyle: UIUserInterfaceStyle {
        get {
            if #available(iOS 13.0, *) {
                return self.overrideUserInterfaceStyle
            }
            return .unspecified
        }
        set {
            if #available(iOS 13.0, *) {
                self.overrideUserInterfaceStyle = newValue
            }
            // Ignore
        }
    }
}

public extension UINavigationBar {
    @objc dynamic var appearanceBarTintColor: UIColor? {
        get { return self.barTintColor }
        set { self.barTintColor = newValue }
    }
}

public extension UIButton {
    @objc dynamic var appearanceTextColor: UIColor! {
        get { return self.titleColor(for: .normal) }
        set {
            self.setTitleColor(newValue, for: .normal)
            // iOS 12 has many problems with overwriting color values.
            // We have to set text color directly to the label as well.
            if #available(iOS 13.0, *) { } else { titleLabel?.appearanceTextColor = newValue }
        }
    }
    
    @objc dynamic var appearanceTintColor: UIColor! {
        get { return self.tintColor }
        set { self.tintColor = newValue }
    }
}
