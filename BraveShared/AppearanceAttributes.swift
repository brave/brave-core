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

public extension UISwitch {
    @objc dynamic var appearanceOnTintColor: UIColor? {
        get { return self.onTintColor }
        set { self.onTintColor = newValue }
    }
}

public extension UIView {
    @objc dynamic var appearanceOverrideUserInterfaceStyle: UIUserInterfaceStyle {
        get { self.overrideUserInterfaceStyle }
        set { self.overrideUserInterfaceStyle = newValue }
    }
}

public extension UINavigationBar {
    @objc dynamic var appearanceBarTintColor: UIColor? {
        get { return self.barTintColor }
        set { self.barTintColor = newValue }
    }
}

public extension UIToolbar {
    @objc dynamic var appearanceBarTintColor: UIColor? {
        get { return self.barTintColor }
        set { self.barTintColor = newValue }
    }
}

public extension UIButton {
    @objc dynamic var appearanceTextColor: UIColor! {
        get { return self.titleColor(for: .normal) }
        set { self.setTitleColor(newValue, for: .normal) }
    }
    
    @objc dynamic var appearanceTintColor: UIColor! {
        get { return self.tintColor }
        set { self.tintColor = newValue }
    }
}
