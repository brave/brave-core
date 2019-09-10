// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared

extension Theme {
    func applyAppearanceProperties() {
        
        // `appearance` modifications only impact UI items not current visible

        // important! for privacy concerns, otherwise UI can bleed through
        UIView.appearance(whenContainedInInstancesOf: [BasePasscodeViewController.self]).appearanceBackgroundColor = colors.home
        
        UIToolbar.appearance().tintColor = colors.accent
        UIToolbar.appearance().backgroundColor = colors.footer
        
        UINavigationBar.appearance().tintColor = colors.accent
        UINavigationBar.appearance().appearanceBarTintColor = colors.header
        
        UISwitch.appearance().onTintColor = colors.accent
        
        // This is a subtle "abuse" of theme colors
        // In order to properly style things, `addressBar` has been utilized to offer contrast to `home`/`header`, as many of the themes utilize similar colors.
        // These used colors have been mapped, primarily for table usage, and to understand how table colors relate to each other.
        // Any change to a single tableView property that currently uses one of these will probably have odd behavior and must be thoroughly tested
        
        /// Used as color a table will use as the base (e.g. background)
        let tablePrimaryColor = colors.home
        /// Used to augment `tablePrimaryColor` above
        let tableSecondaryColor = colors.header
        
        // Will become the color for whatever in the table is .clear
        // In some cases this is the header, footer, cell, or a combination of them.
        // Be careful adjusting colors here, and make sure impact is well known
        UITableView.appearance().appearanceBackgroundColor = tablePrimaryColor
        UITableView.appearance().appearanceSeparatorColor = colors.border.withAlphaComponent(colors.transparencies.borderAlpha)
        
        UITableViewCell.appearance().tintColor = colors.accent
        UITableViewCell.appearance().backgroundColor = tableSecondaryColor

        UIView.appearance(whenContainedInInstancesOf: [UITableViewHeaderFooterView.self]).appearanceBackgroundColor = tablePrimaryColor
        
        UILabel.appearance(whenContainedInInstancesOf: [UITableView.self]).appearanceTextColor = colors.tints.home
        
        AddEditHeaderView.appearance().appearanceBackgroundColor = tableSecondaryColor
        UITextField.appearance().appearanceTextColor = colors.tints.home
        UITextField.appearance().keyboardAppearance = isDark ? .dark : .light
        
        // Sync items
        SyncViewController.SyncView.appearance(whenContainedInInstancesOf: [UINavigationController.self]).appearanceBackgroundColor = colors.home
        SyncDeviceTypeButton.appearance().appearanceBackgroundColor = colors.header
        
        // Search
        UIView.appearance(whenContainedInInstancesOf: [SearchViewController.self]).appearanceBackgroundColor = colors.home
        InsetButton.appearance(whenContainedInInstancesOf: [SearchViewController.self]).appearanceBackgroundColor = .clear
        
        InsetButton.appearance(whenContainedInInstancesOf: [SearchSuggestionPromptView.self]).appearanceTextColor = colors.tints.home
        
        if #available(iOS 13.0, *) {
            // Overrides all views inside of itself
            // According to docs, UIWindow override should be enough, but some labels on iOS 13 are still messed up without UIView override as well
            // (e.g. shields panel)
            UIWindow.appearance().appearanceOverrideUserInterfaceStyle = isDark ? .dark : .light
            UIView.appearance().appearanceOverrideUserInterfaceStyle = isDark ? .dark : .light
        } else {
            // iOS 12 fixes, many styling items do not work properly in iOS 12
            UILabel.appearance().appearanceTextColor = colors.tints.home
            
            // iOS 12 does not allow in-line color overrides :/
            // These UI components are currently non-themed
            // AlertPopupView
            UILabel.appearance(whenContainedInInstancesOf: [AlertPopupView.self]).appearanceTextColor = BraveUX.GreyJ
            UIButton.appearance(whenContainedInInstancesOf: [AlertPopupView.self]).tintColor = .white
            
            // EmptyPrivateTabsView
            UILabel.appearance(whenContainedInInstancesOf: [EmptyPrivateTabsView.self]).appearanceTextColor = UIColor.Photon.Grey10
        }
    }
}

extension UILabel {
    @objc dynamic var appearanceTextColor: UIColor! {
        get { return self.textColor }
        set { self.textColor = newValue }
    }
}

extension InsetButton {
    @objc dynamic var appearanceTextColor: UIColor! {
        get { return self.titleColor(for: .normal) }
        set { self.setTitleColor(newValue, for: .normal) }
    }
}

extension UITableView {
    @objc dynamic var appearanceSeparatorColor: UIColor? {
        get { return self.separatorColor }
        set { self.separatorColor = newValue }
    }
}

extension UIView {
    @objc dynamic var appearanceBackgroundColor: UIColor? {
        get { return self.backgroundColor }
        set { self.backgroundColor = newValue }
    }
}

extension UITextField {
    @objc dynamic var appearanceTextColor: UIColor? {
        get { return self.textColor }
        set { self.textColor = newValue }
    }
}

extension UIView {
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

extension UINavigationBar {
    @objc dynamic var appearanceBarTintColor: UIColor? {
        get { return self.barTintColor }
        set { self.barTintColor = newValue }
    }
}

