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
        UIToolbar.appearance().barTintColor = colors.footer
        
        UINavigationBar.appearance().tintColor = colors.accent
        UINavigationBar.appearance().titleTextAttributes = [.foregroundColor: colors.tints.home]
        UINavigationBar.appearance().largeTitleTextAttributes = [.foregroundColor: colors.tints.home]
        UINavigationBar.appearance().appearanceBarTintColor = colors.header
        
        UISwitch.appearance().appearanceOnTintColor = colors.accent
        
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
        
        UIImageView.appearance(whenContainedInInstancesOf: [SettingsViewController.self]).tintColor = colors.tints.home
        UIImageView.appearance(whenContainedInInstancesOf: [BraveRewardsSettingsViewController.self]).tintColor = colors.tints.home

        UIView.appearance(whenContainedInInstancesOf: [UITableViewHeaderFooterView.self]).appearanceBackgroundColor = tablePrimaryColor
        
        UILabel.appearance(whenContainedInInstancesOf: [UITableView.self]).appearanceTextColor = colors.tints.home
        UILabel.appearance(whenContainedInInstancesOf: [UICollectionReusableView.self]).appearanceTextColor = colors.tints.home
        
        AddEditHeaderView.appearance().appearanceBackgroundColor = tableSecondaryColor
        UITextField.appearance().appearanceTextColor = colors.tints.home
        UITextField.appearance().keyboardAppearance = isDark ? .dark : .light
        
        // Sync items
        SyncViewController.SyncView.appearance(whenContainedInInstancesOf: [UINavigationController.self]).appearanceBackgroundColor = colors.home
        SyncDeviceTypeButton.appearance().appearanceBackgroundColor = colors.header
        UIButton.appearance(
            whenContainedInInstancesOf: [SyncViewController.self]).appearanceTextColor = colors.tints.home
        
        // Search
        UIView.appearance(whenContainedInInstancesOf: [SearchViewController.self]).appearanceBackgroundColor = colors.home
        InsetButton.appearance(whenContainedInInstancesOf: [SearchViewController.self]).appearanceBackgroundColor = .clear
        
        InsetButton.appearance(whenContainedInInstancesOf: [SearchSuggestionPromptView.self]).appearanceTextColor = colors.tints.home
        
        // Overrides all views inside of itself when we're fixed to a specific theme
        // Private browsing mode is also fixed to a specific theme
        if PrivateBrowsingManager.shared.isPrivateBrowsing ||
            Preferences.General.themeNormalMode.value != DefaultTheme.system.rawValue {
            UIWindow.appearance().appearanceOverrideUserInterfaceStyle = isDark ? .dark : .light
            UIView.appearance().appearanceOverrideUserInterfaceStyle = isDark ? .dark : .light
        } else {
            UIWindow.appearance().appearanceOverrideUserInterfaceStyle = .unspecified
            UIView.appearance().appearanceOverrideUserInterfaceStyle = .unspecified
        }
    }
}
