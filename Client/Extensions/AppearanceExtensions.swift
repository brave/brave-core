// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import BraveRewardsUI

extension Theme {
    func applyAppearanceProperties() {
        
        // `appearance` modifications only impact UI items not current visible

        // important! for privacy concerns, otherwise UI can bleed through
        UIView.appearance(whenContainedInInstancesOf: [BasePasscodeViewController.self]).appearanceBackgroundColor = colors.home
        
        UIToolbar.appearance().tintColor = colors.accent
        UIToolbar.appearance().backgroundColor = colors.footer
        UIToolbar.appearance().barTintColor = colors.footer
        
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
        
        UIImageView.appearance(whenContainedInInstancesOf: [SettingsViewController.self]).tintColor = colors.tints.home

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
            UILabel.appearance(whenContainedInInstancesOf: [AlertPopupView.self]).appearanceTextColor = BraveUX.greyJ
            UIButton.appearance(whenContainedInInstancesOf: [AlertPopupView.self]).tintColor = .white
            
            // EmptyPrivateTabsView
            UILabel.appearance(whenContainedInInstancesOf: [EmptyPrivateTabsView.self]).appearanceTextColor = UIColor.Photon.grey10
            
            // See #1548.
            // Using tint color of iOS 13 UISwitch to match better with our light theme
            UISwitch.appearance().tintColor = #colorLiteral(red: 0.8392156863, green: 0.8392156863, blue: 0.8431372549, alpha: 1)
        }
        
        // Brave Rewards
        
        // on iOS 12 global UILabel appearance takes over `barTint` and other properties for some reason.
        // Adding a more specific proxy resolves it.
        if #available(iOS 13, *) { } else {
            UILabel.appearance(whenContainedInInstancesOf: [UINavigationBar.self, RewardsPanelController.self]).appearanceTextColor = .black
        }
        
        // This solves bunch of small theming problems like disclosure indicators color, cell highlight color..
        UIView.appearance(whenContainedInInstancesOf: [RewardsPanelController.self]).appearanceOverrideUserInterfaceStyle = .light
    }
}
